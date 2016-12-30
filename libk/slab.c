#include "slab.h"

#if INTERFACE
#include <stddef.h>
#include <stdint.h>

typedef struct slab_type {
	uint32_t magic;
	size_t esize;
	struct slab * first;
	struct slab_type * next, * prev;
	void (*finalize)(void *);
	void (*mark)(void *);
} slab_type_t;

#endif

typedef struct slab {
	uint32_t magic;
	struct slab * next, * prev;
	slab_type_t * type;
	size_t esize;
	uint32_t available[8];
} slab_t;

static slab_type_t * types;

void slab_type_create(slab_type_t * stype, size_t esize)
{
	stype->first = 0;
	stype->esize = esize;
	stype->finalize = 0;
	stype->mark = 0;
	stype->magic = 997 * 0xaf653de9 * (uint32_t)stype;
	LIST_APPEND(types, stype);
}

static slab_t * slab_new(slab_type_t * stype)
{
	int i;
	int count = (ARCH_PAGE_SIZE - sizeof(slab_t)) / stype->esize;
	/* Allocate and map page */
	slab_t * slab = page_valloc();

	slab->esize = stype->esize;
	slab->magic = stype->magic;
	slab->type = stype;
	LIST_PREPEND(stype->first, slab);

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
				uint32_t mask = 0x80000000;
#if 0
				if (a & 0x0000ffff) slot += 16, a >>= 16;
				if (a & 0x00ff00ff) slot += 8, a >>= 8;
				if (a & 0x0f0f0f0f) slot += 4, a >>= 4;
				if (a & 0x33333333) slot += 2, a >>= 2;
				if (a & 0x55555555) slot += 1, a >>= 1;
#endif
				while(mask) {
					if (a&mask) {
						break;
					}
					mask>>=1;
					slot++;
				}

				slab->available[i] &= ~(0x80000000 >> (slot));

				return (char*)(slab+1) + slab->esize*slot;
			}
		}

		LIST_NEXT(stype->first,slab);
		if (0 == slab) {
			slab = slab_new(stype);
		}
	}

	return 0;
}

static void slab_mark_available_all(slab_t * slab)
{
	int count = ARCH_PAGE_SIZE/slab->esize;
        for(int i=0; i<count; i+=32) {
                uint32_t mask = ~0 ;
                if (count-i < 32) {
                        mask = ~(mask >> (count-i));
                }
                slab->available[i/32] = mask;
        }

}

static void slab_gc_begin()
{
	slab_type_t * stype = types;

	/* Mark all elements available */
	while(stype) {
		slab_t * slab = stype->first;

		while(slab) {
			slab_mark_available_all(slab);
			LIST_NEXT(stype->first, slab);
		}

		LIST_NEXT(types, stype);
	}
}

static void slab_gc_mark(void * root)
{
}

static void slab_gc_end()
{
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
	slab_type_t * t2;
	void * p[4];

	slab_type_create(&t, sizeof(t));
	t2 = slab_alloc(&t);
	slab_type_create(t2, 1270);

	p[0] = slab_alloc(t2);
	p[1] = slab_alloc(t2);
	p[2] = slab_alloc(t2);
	p[3] = slab_alloc(t2);

	slab_free(p[3]);
	slab_free(p[2]);
	slab_free(p[1]);
	slab_free(p[0]);

	slab_free(t2);
}
