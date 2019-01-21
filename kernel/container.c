#include "container.h"

#if INTERFACE

struct container_t {
        pid_t nextpid;
        map_t * pids;
};

#endif

/*
 * List of containers - 0 is root container
 */
static map_t * containers; 

void container_init()
{
	INIT_ONCE();

	/* Create the root container */
	container_t * container = malloc(sizeof(*container));
	container->nextpid = 0;
	container->pids = tree_new(0, TREE_TREAP);

	containers = vector_new();
	map_putip(containers, 0, container);

	/* Make containers a GC root */
	thread_gc_root(containers);
}

static int lock[] = {0};

void container_nextpid( process_t * process )
{
        container_t * container = process->container;

	SPIN_AUTOLOCK(lock) {
		do {
			 process->pid = container->nextpid++;
		} while(map_getip(container->pids, process->pid));
		map_putip(container->pids, process->pid, process);
	}
}

process_t * container_getprocess(container_t * container, pid_t pid)
{
	process_t * process = 0;

	SPIN_AUTOLOCK(lock) {
		process = map_getip(container->pids, pid);
	}

	return process;
}

void container_endprocess( process_t * process )
{
        container_t * container = process->container;

	SPIN_AUTOLOCK(lock) {
		map_removeip(container->pids, process->pid);
	}
}

container_t * container_get(int id)
{
	return map_getip(containers, id);
}
