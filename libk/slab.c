#include "slab.h"

#if INTERFACE
#include <stddef.h>
#include <stdint.h>

typedef struct slab_type {
	uint32_t magic;
	size_t esize;
	struct slab * first;
	struct slab_type * next, * prev;
	void (*mark)(void *);
	void (*finalize)(void *);
} slab_type_t;

#endif

typedef struct slab {
	uint32_t magic;
	struct slab * next, * prev;
	slab_type_t * type;
	size_t esize;
	uint32_t available[8];
	uint32_t finalize[8];
} slab_t;

static slab_type_t * types;

void slab_type_create(slab_type_t * stype, size_t esize, void (*mark)(void *), void (*finalize)(void *))
{
	stype->first = 0;
	stype->esize = esize;
	stype->mark = mark;
	stype->finalize = finalize;
	stype->magic = 997 * 0xaf653de9 * (uint32_t)stype;
	LIST_APPEND(types, stype);
}

static slab_t * slab_new(slab_type_t * stype)
{
	int count = (ARCH_PAGE_SIZE-sizeof(slab_t)) / stype->esize;
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
	for(int i=0; i<count; i+=32) {
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
	thread_lock(slab_alloc);

	slab_t * slab = stype->first ? stype->first : slab_new(stype);

	while(slab) {
		for(int i=0; i<sizeof(slab->available)/sizeof(slab->available[0]); i++) {
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

				slab->available[i] &= ~mask;

				thread_unlock(slab_alloc);
				return (char*)(slab+1) + slab->esize*slot;
			}
		}

		LIST_NEXT(stype->first,slab);
		if (0 == slab) {
			slab = slab_new(stype);
		}
	}

	thread_unlock(slab_alloc);
	/* FIXME: Throw out of memory error */
	return 0;
}

static void slab_mark_available_all(slab_t * slab)
{
	int count = (ARCH_PAGE_SIZE-sizeof(*slab))/slab->esize;
        for(int i=0; i<count; i+=32) {
                uint32_t mask = ~0 ;
                if (count-i < 32) {
                        mask = ~(mask >> (count-i));
                }
		slab->finalize[i/32] = slab->available[i/32];
                slab->available[i/32] = mask;
        }

}

void slab_gc_begin()
{
	thread_lock(slab_alloc);
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

static slab_t * slab_get(void * p)
{
	if (arch_is_heap_pointer(p)) {
		/* Check magic numbers */
		slab_t * slab = ARCH_PAGE_ALIGN(p);

		if (slab->magic == slab->type->magic && (void*)slab < p) {
			return slab;
		}
	}

	return 0;
}

void slab_gc_mark(void * root)
{
	slab_t * slab = slab_get(root);

	if (slab) {
		char * cp = root;
		int i = (cp - (char*)(slab+1)) / slab->esize;
		int mask = (0x80000000 >> i%32);
		if (slab->available[i/32] & mask) {
			/* Marked as available, clear the mark */
			slab->available[i/32] &= ~mask;
			if (slab->type->mark) {
				/* Call type specific mark */
				slab->type->mark(root);
			} else {
				/* Call the generic conservative mark */
				void ** p = (void**)root;
				for(;p<(void**)root+slab->esize/sizeof(void*); p++) {
					slab_gc_mark(*p);
				}
			}
		}
	}
}

void slab_gc_mark_block(void ** block, size_t size)
{
	for(int i=0; i<size/sizeof(*block); i++) {
		slab_gc_mark(block[i]);
	}
}

void slab_gc_mark_range(void ** from, void ** to)
{
	void ** mark = from;
	while(mark<to) {
		slab_gc_mark(*mark++);
	}
}

static void slab_finalize_clear_param(void * param)
{
	param = 0;
}

static void slab_finalize(slab_t * slab)
{
	int count = (ARCH_PAGE_SIZE-sizeof(*slab))/slab->esize;
        for(int i=0; i<count; i+=32) {
		slab->finalize[i/32] ^= slab->available[i/32];
	}
        for(int i=0; i<count; i++) {
		uint32_t mask = 0x80000000 >> (i & 31);
		if (slab->finalize[i/32] & mask) {
			slab->type->finalize((char*)(slab+1) + slab->esize*i);
		}
	}
	/* Clear parameter values left on stack */
	slab_finalize_clear_param(0);
}

void slab_gc_end()
{
	slab_type_t * stype = types;

	/* Finalize elements now available */
	while(stype) {
		slab_t * slab = stype->first;

		while(slab && stype->finalize) {
			slab_finalize(slab);
			LIST_NEXT(stype->first, slab);
		}

		LIST_NEXT(types, stype);
	}
	thread_unlock(slab_alloc);
}

void slab_free(void * p)
{
	slab_t * slab = slab_get(p);

	if (slab) {
		char * cp = p;
		int i = (cp - (char*)(slab+1)) / slab->esize;
		slab->available[i/32] |= (0x80000000 >> i%32);
	}
}

static void slab_test_finalize(void * p)
{
	kernel_printk("Finalizing: 0x%p\n", p);
#if 0
	/* Ensure we don't leave an accidental reference on the stack */
	p = 0;
#endif
}

static void slab_test_mark(void *p)
{
	kernel_printk("Marking: 0x%p\n", p);
#if 0
	/* Ensure we don't leave an accidental reference on the stack */
	p = 0;
#endif
}

void slab_test()
{
	slab_type_t t;
	slab_type_t * t2;
	void * p[4];

	slab_type_create(&t, sizeof(t), 0, slab_test_finalize);
	t2 = slab_alloc(&t);
	slab_type_create(t2, 1270, slab_test_mark, slab_test_finalize);

	p[0] = slab_alloc(t2);
	p[1] = slab_alloc(t2);
	p[2] = slab_alloc(t2);
	p[3] = slab_alloc(t2);

	/* Nothing should be finalized here */
	thread_gc();
	p[0] = p[1] = p[2] = p[3] = 0;

	/* p array should be finalized here */
	thread_gc();
	t2 = 0;

	/* t2 should be finalized here */
	thread_gc();

	slab_free(t2);
}
