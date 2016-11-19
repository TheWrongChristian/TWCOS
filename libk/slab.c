#include <stddef.h>
#include <stdint.h>
#include "slab.h"

#if INTERFACE

typedef struct {
	uint32_t magic;
	size_t esize;
	struct _slab_t * first;
	void (*finalize)(void *);
	void (*mark)(void *);
} slab_type_t;

#endif

typedef struct _slab_t {
	uint32_t magic;
	struct _slab_t * next;
	slab_type_t * type;
	size_t esize;
	uint32_t available[8];
} slab_t;

void slab_type_create(slab_type_t * stype, size_t esize)
{
	stype->first = 0;
	stype->esize = esize;
	stype->finalize = 0;
	stype->mark = 0;
	stype->magic = 997 * 0xaf653de9 * (uint32_t)stype;
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
	slab->magic = stype->magic;
	slab->type = stype;
	stype->first = slab;

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
	slab_t * slab = stype->first ? stype->first : slab_new(stype);

	while(slab) {
		int i=0;

		for(i=0; i<sizeof(slab->available)/sizeof(slab->available[0]); i++) {
			if (slab->available[i]) {
				/* There is some available slots */
				int slot = i*32;
				uint32_t a = slab->available[i];

				if (a & 0x0000ffff) slot += 16;
				if (a & 0x00ff00ff) slot += 8;
				if (a & 0x0f0f0f0f) slot += 4;
				if (a & 0x33333333) slot += 2;
				if (a & 0x55555555) slot += 1;

				slab->available[i] &= ~(0x80000000 >> slot);

				return (char*)(slab+1) + slab->esize*slot;
			}
		}

		if (slab->next) {
			slab = slab->next;
		} else {
			slab = slab_new(stype);
		}
	}

	return 0;
}

void slab_free(void * p)
{
	/* Check magic numbers */
	slab_t * slab = ARCH_PAGE_ALIGN(p);

	if (slab->magic == slab->type->magic) {
		char * cp = p;
		int i = (cp - (char*)(slab+1)) / slab->esize;
		slab->available[i/32] |= (0x80000000 >> i%32);
	}
}

void slab_test()
{
	slab_type_t t;

	slab_type_create(&t, sizeof(t));
	slab_free(slab_alloc(&t));
}
