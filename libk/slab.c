#include "slab.h"

#if INTERFACE
#include <stddef.h>
#include <stdint.h>

typedef struct slab_type {
	uint32_t magic;
	size_t esize;
	int count;
	struct slab * first;
	struct slab_type * next, * prev;
	void (*mark)(void *);
	void (*finalize)(void *);
} slab_type_t;

typedef struct {
	void * p;
} slab_weak_ref_t;

#endif

typedef struct slab {
	uint32_t magic;
	struct slab * next, * prev;
	slab_type_t * type;
	uint32_t * available;
	uint32_t * finalize;
	char * data;
} slab_t;

static slab_type_t * types;

void slab_type_create(slab_type_t * stype, size_t esize, void (*mark)(void *), void (*finalize)(void *))
{
	stype->first = 0;
	stype->esize = esize;
	stype->mark = mark;
	stype->finalize = finalize;
	stype->magic = 997 * 0xaf653de9 * (uint32_t)stype;

	/*           <-----------------d------------------>
	 * | slab_t |a|f|              data                |
	 *  <-----------------page size------------------->
	 * data + a + f = ARCH_PAGE_SIZE-sizeof(slab_t)
	 * c*s + c/8+4 + c/8+4 = psz-slab_t = d
	 * 8*c*s + c + 32 + c + 32 = 8*d
	 * 8*c*s + 2*c = 8*d - 64
	 * c*(8*s + 2) = 8*d - 64
	 * c = (8*d - 64) / (8*s + 2)
	 */
	stype->count = (8*(ARCH_PAGE_SIZE-sizeof(slab_t))-64)/ (8 * stype->esize + 2);
	LIST_APPEND(types, stype);
}

static void slab_weak_ref_mark(void * p)
{
	/* Intentionally empty */
}

static void slab_weak_ref_finalize(void * p)
{
	slab_weak_ref_t * ref = p;
	ref->p = 0;
}

slab_weak_ref_t * slab_weak_ref(void * p)
{
	static slab_type_t wr[1];
	static int inited = 0;

	thread_lock(slab_alloc);

	if (!inited) {
		inited = 1;
		slab_type_create(wr, sizeof(slab_weak_ref_t), slab_weak_ref_mark, slab_weak_ref_finalize);
	}

	thread_unlock(slab_alloc);

	slab_weak_ref_t * ref = slab_alloc(wr);
	ref->p = p;

	return ref;
}

void * slab_weak_ref_get(slab_weak_ref_t * ref)
{
	thread_lock(slab_alloc);
	void * p = ref->p;
	thread_unlock(slab_alloc);
	return p;
}

static slab_t * slab_new(slab_type_t * stype)
{
	/* Allocate and map page */
	slab_t * slab = page_valloc();

	slab->magic = stype->magic;
	slab->type = stype;
	slab->available = (uint32_t*)(slab+1);
	slab->finalize = slab->available + (slab->type->count+32)/32;
	slab->data = (char*)(slab->finalize + (slab->type->count+32)/32);
	LIST_PREPEND(stype->first, slab);

	for(int i=0; i<stype->count; i+=32) {
		uint32_t mask = ~0 ;
		if (stype->count-i < 32) {
			mask = ~(mask >> (stype->count-i));
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
		for(int i=0; i<slab->type->count; i+=32) {
			if (slab->available[i/32]) {
				/* There is some available slots */
				int slot = i;
				uint32_t a = slab->available[i/32];
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

				slab->available[i/32] &= ~mask;

				thread_unlock(slab_alloc);
				return slab->data + slab->type->esize*slot;
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

void * slab_calloc(slab_type_t * stype)
{
	void * p = slab_alloc(stype);
	memset(p, 0, stype->esize);

	return p;
}

static void slab_mark_available_all(slab_t * slab)
{
        for(int i=0; i<slab->type->count; i+=32) {
                uint32_t mask = ~0 ;
                if (slab->type->count-i < 32) {
                        mask = ~(mask >> (slab->type->count-i));
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

		if (slab == ARCH_PAGE_ALIGN(slab->data) && slab->magic == slab->type->magic && (char*)slab->data <= (char*)p) {
			return slab;
		}
	}

	return 0;
}

void slab_gc_mark(void * root)
{
	slab_t * slab = slab_get(root);

	if (slab) {
		int i = ((char*)root - slab->data) / slab->type->esize;
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
				for(;p<(void**)root+slab->type->esize/sizeof(void*); p++) {
					slab_gc_mark(*p);
				}
				p = 0;
			}
		}
	}
	slab=0;
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
        for(int i=0; i<slab->type->count; i+=32) {
		slab->finalize[i/32] ^= slab->available[i/32];
	}
        for(int i=0; i<slab->type->count; ) {
		if (slab->finalize[i/32]) {
			uint32_t mask = 0x80000000;
			for(; i<slab->type->count && mask; i++, mask>>=1) {
				if (slab->finalize[i/32] & mask) {
					slab->type->finalize(slab->data + slab->type->esize*i);
				}
			}
		} else {
			i+=32;
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
		int i = (cp - slab->data) / slab->type->esize;
		slab->available[i/32] |= (0x80000000 >> i%32);
		if (slab->type->finalize) {
			slab->type->finalize(p);
		}
		p = 0;
	}
}

static void slab_test_finalize(void * p)
{
	kernel_printk("Finalizing: %p\n", p);
}

static void slab_test_mark(void *p)
{
	kernel_printk("Marking: %p\n", p);
}

void slab_test()
{
	static slab_type_t t[1];
	void * p[4];

	slab_type_create(t, 1270, slab_test_mark, slab_test_finalize);

	p[0] = slab_alloc(t);
	slab_weak_ref_t * ref = slab_weak_ref(p[0]);
	p[1] = slab_alloc(t);
	p[2] = slab_alloc(t);
	p[3] = slab_alloc(t);

	/* Nothing should be finalized here */
	thread_gc();
	kernel_printk("Weak p[0] = 0x%p\n", slab_weak_ref_get(ref));
	p[0] = p[1] = p[2] = p[3] = 0;

	/* p array should be finalized here */
	thread_gc();
	kernel_printk("Weak p[0] = 0x%p\n", slab_weak_ref_get(ref));
}
