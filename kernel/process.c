#include "process.h"

#if INTERFACE

#include <stdint.h>

typedef uint32_t pid_t;

#define WNOHANG 1

struct process_t {
	monitor_t lock;

	pid_t pid;
#if 0
	credential_t * credentials;
#endif

	/*
	 * Process resources
	 */
	map_t * as;
	map_t * files;

	/*
	 * root and working directories
	 */
	vnode_t * root;
	vnode_t * cwd;

	/*
	 * Threads
	 */
	map_t * threads;

	/*
	 * process hierarchy
	 */
	process_t * parent;
	map_t * children;
	map_t * zombies;

	/*
	 * Status
	 */
	int exitcode;

	container_t * container;
};


#endif

process_t * process_get()
{
	return arch_get_thread()->process;
}

map_t * process_files()
{
	process_t * process = process_get();

	return process->files;
}

static void process_duplicate_as_copy_seg(void * p, void * key, void * data)
{
	map_t * as = (map_t*)p;
	segment_t * seg = (segment_t *)data;

	int private = (seg->clean != seg->dirty);
	map_putpp(as, key, vm_segment_copy(seg, private));
}

static map_t * process_duplicate_as(process_t * from)
{
	map_t * as = tree_new(0, TREE_TREAP);

	map_walkpp(from->as, process_duplicate_as_copy_seg, as);

	return as;
}

slab_type_t processes[] = {SLAB_TYPE(sizeof(process_t), 0, 0)};

void process_init()
{
	INIT_ONCE();

	container_init();
	thread_t * current = arch_get_thread();

	/* Sculpt initial process */
	current->process = slab_calloc(processes);
	current->process->as = tree_new(0, TREE_TREAP);
	current->process->threads = tree_new(0, TREE_TREAP);
	current->process->children = tree_new(0, TREE_TREAP);
	current->process->zombies = tree_new(0, TREE_TREAP);
	map_putpp(current->process->threads, current, current);
	current->process->container = container_get(0);
	current->process->files = vector_new();
	container_nextpid(current->process);
}

pid_t process_fork()
{
	process_t * current = process_get();
	process_t * new = slab_calloc(processes);

	/* Share the same container */
	new->container = current->container;

	/* New address space */
	new->as = process_duplicate_as(current);

	/* Thread set */
	new->threads = tree_new(0, TREE_TREAP);

	/* Copy of all file descriptors */
	new->files = vector_new();
	map_put_all(new->files, current->files);

	/* Find an unused pid */
	container_nextpid(new);

	/* Process hierarchy */
	new->children = tree_new(0, TREE_TREAP);
	new->zombies = tree_new(0, TREE_TREAP);
	new->parent = current;
	map_putip(current->children, new->pid, new);

	/* Directories */
	new->root = current->root;
	new->cwd = current->cwd;

	/* Finally, new thread */
	thread_t * thread = thread_fork();
	if (0 == thread) {
		/* Child thread */
		return 0;
	} else {
		thread->process = new;
		map_putpp(thread->process->threads, thread, thread);
	}

	return new->pid;
}

static void process_exit_reparent(void * p, map_key key, void * data)
{
	process_t * current = data;
	current->parent = p;
}

void process_exit(int code)
{
	process_t * current = process_get();
	process_t * init = container_getprocess(current->container, 1);

	if (init == current) {
		kernel_panic("init: exiting!");
	}

	MONITOR_AUTOLOCK(&current->parent->lock) {
		pid_t pid = current->pid;
		current->exitcode = code;
		map_putip(current->parent->zombies, pid, map_removeip(current->parent->children, pid));
		monitor_signal(&current->parent->lock);

		/* Pass off any child/zombie processes to init */
		MONITOR_AUTOLOCK(&current->lock) {
			MONITOR_AUTOLOCK(&init->lock) {
				map_walkip(current->children, process_exit_reparent, init);
				map_put_all(init->children, current->children);
				map_walkip(current->zombies, process_exit_reparent, init);
				map_put_all(init->zombies, current->zombies);
			}
		}
	}

	/* FIXME: Clean up address space, files, other threads etc. */
	thread_exit(0);
}

static void process_waitpid_getzombie(void * p, map_key key, void * data)
{
	longjmp(p, key);
}

pid_t process_waitpid(pid_t pid, int * wstatus, int options)
{
	int hang = 0 == (options&WNOHANG);
	pid_t child = 0;
	process_t * current = process_get();

	MONITOR_AUTOLOCK(&current->lock) {
#if 0
		if (-1 == pid) {
			/* Any child */
		} else if (pid<0) {
			/* Any child in process group -pid */
		} else if (0 == pid) {
			child = pid;
		} else {
		}
#endif
		do {
			if (pid>0) {
				child=pid;
			} else {
				jmp_buf env;
				child = setjmp(env);
				if (0 == child) {
					map_walkip(current->zombies, process_waitpid_getzombie, env);
				}
			}
			if (0 == child && hang) {
				monitor_wait(&current->lock);
			}
		} while(0 == child && hang);

		if (child) {
			process_t * process = map_removeip(current->zombies, child);
			if (wstatus) {
				*wstatus = process->exitcode;
			}

			/* Finally remove the process from the set of all processes */
			container_endprocess(process);
		}
	}

	return child;
}

void process_execve(char * filename, char * argv[], char * envp[])
{
	vnode_t * f = file_namev(filename);
	process_t * p = arch_get_thread()->process;

	/* Save old AS */
	map_t * oldas = p->as;

	KTRY {
		elf_execve(f, p, argv, envp);
	} KCATCH(Exception) {
		/* Restore old AS */
		p->as = oldas;
		vmap_set_asid(p->as);

		/* Propagate original exception */
		KRETHROW();
	}
}
