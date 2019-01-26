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
	mutex_t lock[1];
} slab_type_t;

#define SLAB_TYPE(s, m, f) {.magic=0, .esize=s, .mark=m, .finalize=f}

#ifdef DEBUG
#define malloc(size) malloc_d(size, __FILE__, __LINE__)
#define calloc(count, size) calloc_d(count, size, __FILE__, __LINE__)
#define realloc(p, size) realloc_d(p, size, __FILE__, __LINE__)
#define slab_alloc(type) slab_alloc_d(type, __FILE__, __LINE__)
#define slab_calloc(type) slab_calloc_d(type, __FILE__, __LINE__)
#else
#define malloc(size) malloc_p(size)
#define calloc(count, size) calloc_p(count, size)
#define realloc(p, size) realloc_p(p, size)
#define slab_alloc(type) slab_alloc_p(type)
#define slab_calloc(type) slab_calloc_p(type)
#endif

#endif

exception_def OutOfMemoryException = { "OutOfMemoryException", &Exception };
exception_def AllocationTooBigException = { "AllocationTooBigException", &Exception };

typedef struct slab {
	uint32_t magic;
	struct slab * next, * prev;
	slab_type_t * type;
	uint32_t * available;
	uint32_t * finalize;
	char * data;
} slab_t;

static slab_type_t * types;

void slab_init()
{
	INIT_ONCE();
}

static slab_t * slab_new(slab_type_t * stype)
{
	/* Allocate and map page */
	slab_t * slab = page_heap_alloc();

	if (0 == stype->magic) {
		/* Initialize type */
		stype->first = 0;
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

static rwlock_t slablock[1];
static void slab_lock(int gc)
{
	if (gc) {
		rwlock_write(slablock);
	} else {
		rwlock_read(slablock);
	}
}

static void slab_unlock()
{
	rwlock_unlock(slablock);
}

void * slab_alloc_p(slab_type_t * stype)
{
	slab_lock(0);
	mutex_lock(stype->lock);
	slab_t * slab = stype->first ? stype->first : slab_new(stype);

	while(slab) {
		assert(stype == slab->type);
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
				mutex_unlock(stype->lock);
				slab_unlock();
				return slab->data + slab->type->esize*slot;
			}
		}

		LIST_NEXT(stype->first,slab);
		if (0 == slab) {
			slab = slab_new(stype);
		}
	}

	mutex_unlock(stype->lock);
	slab_unlock();

	KTHROW(OutOfMemoryException, "Out of memory");
	/* Shouldn't get here */
	return 0;
}

void * slab_calloc_p(slab_type_t * stype)
{
	void * p = slab_alloc_p(stype);
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
	slab_lock(1);
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

void slab_gc_mark_range(void * from, void * to)
{
	void ** mark = (void**)from;
	while((void*)mark<to) {
		slab_gc_mark(*mark++);
	}
}

static void slab_finalize_clear_param(void * param)
{
	param = 0;
}

int finalizer_debug_val;
static void debug_finalizer(slab_t * slab)
{
	finalizer_debug_val=1;
}

static void slab_finalize(slab_t * slab)
{
        for(int i=0; i<slab->type->count; i+=32) {
		slab->finalize[i/32] ^= slab->available[i/32];
	}
        for(int i=0; i<slab->type->count; ) {
		if (slab->finalize[i/32]) {
			debug_finalizer(slab);
			uint32_t finalize = slab->finalize[i/32];
			uint32_t mask = 0x80000000;
			slab->finalize[i/32] = 0;
			for(; i<slab->type->count && mask; i++, mask>>=1) {
				if (finalize & mask) {
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
	slab_unlock();
}

void slab_free(void * p)
{
#if 0
	slab_t * slab = slab_get(p);

	if (slab) {
		slab_lock();
		char * cp = p;
		int i = (cp - slab->data) / slab->type->esize;
		slab->available[i/32] |= (0x80000000 >> i%32);
		if (slab->type->finalize) {
			slab->type->finalize(p);
		}
		p = 0;
		slab_unlock();
	}
#endif
}

static void slab_test_finalize(void * p)
{
	kernel_printk("Finalizing: %p\n", p);
}

static void slab_test_mark(void *p)
{
	kernel_printk("Marking: %p\n", p);
}

static void slab_malloc_finalize(void * p)
{
}

static slab_type_t pools[] = {
	SLAB_TYPE(8, 0, slab_malloc_finalize),
	SLAB_TYPE(12, 0, slab_malloc_finalize),
	SLAB_TYPE(16, 0, slab_malloc_finalize),
	SLAB_TYPE(24, 0, slab_malloc_finalize),
	SLAB_TYPE(32, 0, slab_malloc_finalize),
	SLAB_TYPE(48, 0, slab_malloc_finalize),
	SLAB_TYPE(64, 0, slab_malloc_finalize),
	SLAB_TYPE(96, 0, slab_malloc_finalize),
	SLAB_TYPE(128, 0, slab_malloc_finalize),
	SLAB_TYPE(196, 0, slab_malloc_finalize),
	SLAB_TYPE(256, 0, slab_malloc_finalize),
	SLAB_TYPE(384, 0, slab_malloc_finalize),
	SLAB_TYPE(512, 0, slab_malloc_finalize),
	SLAB_TYPE(768, 0, slab_malloc_finalize),
	SLAB_TYPE(1024, 0, slab_malloc_finalize),
	SLAB_TYPE(1536, 0, slab_malloc_finalize),
	SLAB_TYPE((ARCH_PAGE_SIZE-2*sizeof(uint32_t))/2, 0, slab_malloc_finalize),
	SLAB_TYPE((ARCH_PAGE_SIZE-2*sizeof(uint32_t)), 0, slab_malloc_finalize),
};

#ifdef DEBUG

static struct {
	void * p;
	char * file;
	int line;
	size_t size;
	slab_type_t * type;
} audit[32];

void * add_alloc_audit(void * p, char * file, int line, size_t size, slab_type_t * type)
{
	static int next = 0;

	audit[next].p = p;
	audit[next].file = file;
	audit[next].line = line;
	audit[next].size = size;
	audit[next].type = type;

	if (sizeof(audit)/sizeof(audit[0]) == ++next) {
		next = 0;
	}

	return p;
}

void dump_alloc_audit(void * p)
{
	for(int i=0; i<sizeof(audit)/sizeof(audit[0]); i++) {
		char * cp = (char*)p;
		char * base = (char*)audit[i].p;

		if (cp>=base && cp<base+audit[i].size) {
			kernel_printk("pointer alloc'd at %s:%d\n", audit[i].file, audit[i].line);
		}
	}
}

void * malloc_d(size_t size, char * file, int line)
{
	return add_alloc_audit(malloc_p(size), file, line, size, 0);
}

void * calloc_d(int num, size_t size, char * file, int line)
{
	return add_alloc_audit(calloc_p(num, size), file, line, num*size, 0);
}

void * realloc_d(void * p, size_t size, char * file, int line)
{
	return add_alloc_audit(realloc_p(p, size), file, line, size, 0);
}

void * slab_alloc_d(slab_type_t * stype, char * file, int line)
{
	return add_alloc_audit(slab_alloc_p(stype), file, line, stype->esize, stype);
}

void * slab_calloc_d(slab_type_t * stype, char * file, int line)
{
	return add_alloc_audit(slab_calloc_p(stype), file, line, stype->esize, stype);
}
#else

void dump_alloc_audit(void * p)
{
	kernel_printk("No alloc audit log\n");
}

#endif

void * malloc_p(size_t size)
{
	for(int i=0; i<sizeof(pools)/sizeof(pools[0]);i++) {
		if (pools[i].esize > size) {
			return slab_alloc_p(pools+i);
		}
	}

	KTHROWF(AllocationTooBigException, "Allocation too big for malloc: %d", size);

	/* Never reached */
	return 0;
}

void free(void *p)
{
}

void * calloc_p(size_t num, size_t size)
{
	void * p = malloc_p(num*size);
	if (p) {
		memset(p, 0, num*size);
	}

	return p;
}

void *realloc_p(void *p, size_t size)
{
	if (0 == p) {
		return malloc_p(size);
	}

	slab_t * slab = slab_get(p);

	if (slab) {
		if (size <= slab->type->esize) {
			/* Nothing to do, new memory fits in existing slot */
			return p;
		} else {
			void * new = malloc_p(size);

			/* Copy old data (of old size) to new buffer */
			return memcpy(new, p, slab->type->esize);
		}
	} else {
		/* FIXME: We should do something here to warn of misuse */
		kernel_panic("realloc: Invalid heap pointer: %p\n", p);
	}

	kernel_panic("realloc: we shouldn't get here!");
	return 0;
}

void slab_test()
{
	static slab_type_t t[1] = {SLAB_TYPE(1270, slab_test_mark, slab_test_finalize)};
	void * p[4];

	p[0] = slab_alloc(t);
	p[1] = slab_alloc(t);
	p[2] = slab_alloc(t);
	p[3] = slab_alloc(t);

	/* Nothing should be finalized here */
	thread_gc();
	p[0] = p[1] = p[2] = p[3] = malloc(653);

	/* p array should be finalized here */
	thread_gc();

	p[0] = p[1] = p[2] = p[3] = realloc(p[0], 736);
	p[0] = p[1] = p[2] = p[3] = realloc(p[0], 1736);

	thread_gc();
}
