#include "process.h"

#if INTERFACE

typedef uint32_t pid_t;


struct process_t {
	pid_t pid;
	process_t * parent;
#if 0
	credential_t * credentials;
#endif
	map_t * as;
	map_t * files;

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
	map_t * as = arraymap_new(0, 512);

	map_walk(from->as, process_duplicate_as_copy_seg, as);

	return as;
}


slab_type_t processes[] = {SLAB_TYPE(sizeof(process_t), 0, 0)};

pid_t process_fork()
{
	process_t * current = process_get();
	process_t * new = slab_alloc(processes);

	/* Share the same container */
	new->container = current->container;

	/* New address space */
	new->as = process_duplicate_as(current);

	/* Copy of all file descriptors */
	new->files = vector_new();
	map_put_all(new->files, current->files);
	new->parent = current;

	return 0;
}
