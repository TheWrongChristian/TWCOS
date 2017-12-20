#include "container.h"

#if INTERFACE

struct container_t {
        pid_t nextpid;
        map_t * pids;
        vnode_t * root;
};

#endif

/*
 * List of containers - 0 is root container
 */
static map_t * containers; 

void container_init()
{
	INIT_ONCE();

	containers = vector_new();
	thread_gc_root(containers);
}
