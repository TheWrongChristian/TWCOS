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
	segment_t * heap;

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
	current->process->heap = vm_segment_anonymous(0, 0, SEGMENT_P | SEGMENT_R | SEGMENT_W);
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

	/* New heap */
	if (current->heap) {
		new->heap = vm_get_segment(new->as, current->heap->base);
	}

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
#if 0
		static timerspec_t next = 0;
		/* Child thread */
		next += 10000;
		timerspec_t usec = next - timer_uptime();
		if (usec>0) {
			timer_sleep(usec);
		}
#endif
		/* FIXME: Wait for go-ahead from parent */
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

pid_t process_getpid()
{
	process_t * current = process_get();

	if (current) {
		return current->pid;
	}

	kernel_panic("No process!");
	return 0;
}

int process_exit(int code)
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
			current->children = current->zombies = 0;
		}
	}

	/* FIXME: Clean up address space, files, other threads etc. */
	if (current->as) {
		vm_as_release(current->as);
		current->as = 0;
	}

	thread_exit(0);

	// Not reached
	return 0;
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

int process_execve(char * filename, char * argv[], char * envp[])
{
	vnode_t * f = file_namev(filename);
	process_t * p = arch_get_thread()->process;

	return elf_execve(f, p, argv, envp);
}

void * process_brk(void * p)
{
	process_t * current = process_get();
	void * brk = ((char*)current->heap->base) + current->heap->size;

	if (p <= brk) {
		return brk;
	}

	/* Extend the heap */
	current->heap->size = (uintptr_t)p - (uintptr_t)current->heap->base;

	return p;
}

int process_chdir(const char * path)
{
	process_t * p = arch_get_thread()->process;
	vnode_t * d = file_namev(path);

	if (d && VNODE_DIRECTORY==d->type) {
		p->cwd = d;
	}

	return 0;
}

time_t process_time()
{
	return 0;
}
