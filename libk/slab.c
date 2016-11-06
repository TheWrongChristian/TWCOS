#include "slab.h"

#if INTERFACE

typedef struct {
	slab_t * next;
	size_t esize;
	char * freemap;
	char * data;
} slab_t;

#endif

slab_t slab_create(size_t esize)
{
	
	slab_t * slab;
}
