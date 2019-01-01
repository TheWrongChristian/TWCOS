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

container_t * container_get(int id)
{
	return map_getip(containers, id);
}
