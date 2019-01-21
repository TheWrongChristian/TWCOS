#include "process.h"

#if INTERFACE

#include <stdint.h>

typedef uint32_t pid_t;


struct process_t {
	monitor_t lock;

	pid_t pid;
	pid_t ppid;
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
	map_t * children;

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

	map_putpp(as, key, vm_segment_copy(seg, 1));
}

static map_t * process_duplicate_as(process_t * from)
{
	map_t * as = tree_new(0, TREE_TREAP);

	map_walk(from->as, process_duplicate_as_copy_seg, as);

	return as;
}

slab_type_t processes[] = {SLAB_TYPE(sizeof(process_t), 0, 0)};

void process_init()
{
	INIT_ONCE();

	container_init();
	thread_t * current = arch_get_thread();

	/* Sculpt initial process */
	current->process = slab_alloc(processes);
	current->process->as = tree_new(0, TREE_TREAP);
	current->process->threads = tree_new(0, TREE_TREAP);
	current->process->children = tree_new(0, TREE_TREAP);
	map_putpp(current->process->threads, current, current);
	current->process->container = container_get(0);
	current->process->files = vector_new();
	container_nextpid(current->process);
}

pid_t process_fork()
{
	process_t * current = process_get();
	process_t * new = slab_alloc(processes);

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
	new->ppid = current->pid;
	map_putip(current->children, new->pid, new);

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

void process_exit(int code)
{
	process_t * current = process_get();
	process_t * parent = container_getprocess(current->container, current->ppid);
	process_t * init = container_getprocess(current->container, 1);

	MONITOR_AUTOLOCK(&parent->lock) {
		current->exitcode = code;
		container_endprocess(current);
		monitor_signal(&parent->lock);
	}
}

static void waitpid_find_child(void * p, map_key key, void * data)
{
	process_t * child = data;

	if (child->exitcode) {
		longjmp(p, child->pid);
	}
}

pid_t process_waitpid(pid_t pid, int * wstatus, int options)
{
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
				jmp_buf buf;
				child = setjmp(buf);
				map_walkip(current->children, waitpid_find_child, buf);
				monitor_wait(&current->lock);
			}
		} while(0 == child);
	}

	return child;
}
