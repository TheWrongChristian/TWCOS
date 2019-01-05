#include "process.h"

#if INTERFACE

#include <stdint.h>

typedef uint32_t pid_t;


struct process_t {
	pid_t pid;
	process_t * parent;
#if 0
	credential_t * credentials;
#endif
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

static void process_nextpid( process_t * process )
{
	static int lock[] = {0};
	container_t * container = process->container;

	spin_lock(lock);
	do {
		 process->pid = container->nextpid++;
	} while(map_getip(container->pids, process->pid));
	map_putip(container->pids, process->pid, process);
	spin_unlock(lock);
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
	map_putpp(current->process->threads, current, current);
	current->process->parent = 0;
	current->process->container = container_get(0);
	current->process->files = vector_new();
	process_nextpid(current->process);
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
	new->parent = current;

	/* Find an unused pid */
	process_nextpid(new);

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
