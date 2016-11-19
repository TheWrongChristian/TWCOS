#include <stddef.h>
#include <stdint.h>
#include "slab.h"

#if INTERFACE

typedef struct {
	size_t esize;
	struct _slab_t * first;
	void (*finalize)(void *);
	void (*mark)(void *);
} slab_type_t;

#endif

typedef struct _slab_t {
	struct _slab_t * next;
	size_t esize;
	uint32_t available[8];
	char data[1];
} slab_t;

void slab_type_create(slab_type_t * stype, size_t esize)
{
	stype->first = 0;
	stype->esize = esize;
	stype->finalize = 0;
	stype->mark = 0;
}

static slab_t * slab_new(slab_type_t * stype)
{
	int i;
	int count = (ARCH_PAGE_SIZE - sizeof(slab_t)) / stype->esize;
	/* Allocate and map page */
	page_t page = page_alloc();
	slab_t * slab = arch_heap_page();
	vmap_map(0, slab, page);

	slab->esize = stype->esize;
	slab->next = stype->first;

	/*
	 * Up to 256 elements per slab
	 */
	if (count>256) {
		count = 256;
	}
	for(i=0; i<count; i+=32) {
		uint32_t mask = ~0 ;
		if (count-i < 32) {
			mask = ~(mask >> (count-i));
		}
		slab->available[i/32] = mask;
	}

	return slab;
}

void * slab_alloc(slab_type_t * stype)
{
	slab_t * slab = stype->first;

	while(slab) {
		int i=0;
		
	}
	slab = slab_new(stype);

	return 0;
}

void slab_test()
{
	slab_type_t t;

	slab_type_create(&t, sizeof(t));
	slab_alloc(&t);
}
