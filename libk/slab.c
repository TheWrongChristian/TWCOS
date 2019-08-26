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
	vmpage_t vmpage[1];
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

static thread_t * cleaner_thread;
int is_cleaner()
{
	return arch_get_thread() == cleaner_thread;
}

static void cleaner()
{
	cleaner_thread = arch_get_thread();
	int pages = page_count();
	while(1) {
		MONITOR_AUTOLOCK(cleansignal) {
			monitor_wait(cleansignal);
		}
		int prefree = page_count_free();
		thread_gc();
		int postfree = page_count_free();
		MONITOR_AUTOLOCK(freesignal) {
			monitor_broadcast(freesignal);
		}
	}
}

void slab_init()
{
	INIT_ONCE();
	kernel_startlogging(1);
#if 0
	if (0 == thread_fork()) {
		cleaner();
	}
#endif
}

static slab_t * slab_new(slab_type_t * stype)
{
	/* Allocate and map page */
	slab_t * slab = page_heap_alloc();

	if (0 == stype->magic) {
		/* Initialize type */
		stype->first = 0;
		stype->magic = 997 * (uint32_t)stype;

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

	bitarray_setall(slab->available, stype->count, 1);

	assert(slab->type);

	return slab;
}

static mutex_t slablock[1];
static void slab_lock(int gc)
{
	if (gc) {
		mutex_lock(slablock);
	} else {
		mutex_lock(slablock);
	}
}

static void slab_unlock()
{
	mutex_unlock(slablock);
}

static void debug_fillbuf(void * p, size_t l, int c)
{
	memset(p, c, l);
}

static void debug_checkbuf(void * p, size_t l)
{
	const char * cp = p;
	char c = *cp;

	for(int i=0; i<l; i++)
	{
		if (c != *cp) {
			kernel_panic("Corrupted buffer: %p", p);
		}
	}
}


void * slab_alloc_p(slab_type_t * stype)
{
	mutex_lock(stype->lock);
	slab_t * slab = stype->first ? stype->first : slab_new(stype);

	slab_lock(0);
	while(slab) {
		assert(stype == slab->type);
		int slot = bitarray_firstset(slab->available, slab->type->count);
		if (slot>=0) {
			bitarray_set(slab->available, slot, 0);
			bitarray_set(slab->finalize, slot, 0);
			slab_unlock();
			mutex_unlock(stype->lock);
			debug_checkbuf(slab->data + slab->type->esize*slot, slab->type->esize);
			debug_fillbuf(slab->data + slab->type->esize*slot, slab->type->esize, 0x0a);
			return slab->data + slab->type->esize*slot;
		}

		LIST_NEXT(stype->first,slab);
		if (0 == slab) {
			slab = slab_new(stype);
		}
	}

	slab_unlock();
	mutex_unlock(stype->lock);

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

static int slab_all_free(slab_t * slab)
{
        for(int i=0; i<slab->type->count; i+=32) {
                uint32_t mask = ~0 ;
                if (slab->type->count-i < 32) {
                        mask = ~(mask >> (slab->type->count-i));
                }
		if (mask ^ slab->available[i/32]) {
			return 0;
		}
	}

	return 1;
}

void slab_nomark(void * p)
{
	/* Does nothing */
}

static struct
{
	size_t inuse;
	size_t peak;
	size_t total;
} gc_stats = {0};

static struct gccontext {
	/* The block to be scanned */
	void ** from;
	void ** to;

	/* Previous context */
	arena_state state;
	struct gccontext * prev;
} * context = 0;
static arena_t * gcarena = 0;
static int gclevel = 0;

void slab_gc_begin()
{
	slab_lock(1);
	slab_type_t * stype = types;

	gc_stats.inuse = 0;
	gc_stats.total = 0;

	/* Mark all elements available */
	while(stype) {
		slab_t * slab = stype->first;

		while(slab) {
			gc_stats.total += stype->esize * stype->count;
			/* Provisionally finalize all allocated slots */
			bitarray_copy(slab->finalize, slab->available, stype->count);
			bitarray_invert(slab->finalize, stype->count);
			LIST_NEXT(stype->first, slab);
		}

		LIST_NEXT(types, stype);
	}
	gcarena = arena_get();
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
		/* Entry within the slab */
		int entry = ((char*)root - slab->data) / slab->type->esize;

		/* Adjust root to point to the start of the slab entry */
		root = slab->data + slab->type->esize*entry;

		if (bitarray_get(slab->finalize, entry)) {
			/* Marked for finalization, clear the mark */
			bitarray_set(slab->finalize, entry, 0);
			gc_stats.inuse += slab->type->esize;

			if (slab->type->mark) {
				/* Call type specific mark */
				slab->type->mark(root);
			} else {
				/* Generic mark */
				struct gccontext * new = arena_calloc(gcarena, sizeof(*new));
				new->state = arena_getstate(gcarena);
				new->from = root;
				new->to = new->from + slab->type->esize/sizeof(*new->to);
				new->prev = context;
				context = new;
				gclevel++;
			}
		}
	}
}

void slab_gc_mark_range(void * from, void * to)
{
	struct gccontext * new = arena_calloc(gcarena, sizeof(*new));
	new->from = from;
	new->to = to;
	new->state = arena_getstate(gcarena);
	new->prev = context;
	context = new;
	gclevel++;
}

void slab_gc()
{
	mutex_t lock[1] = {0};

	MUTEX_AUTOLOCK(lock) {
		arena_state state = arena_getstate(gcarena);

		while(context) {
			/* Check the next pointer in the block */
			if (context->from && context->from < context->to) {
				slab_gc_mark(*context->from++);
			} else {
				arena_setstate(gcarena, context->state);
				context = context->prev;
				gclevel--;
			}
		}
	}
}

void slab_gc_mark_block(void ** block, size_t size)
{
	slab_gc_mark_range(block, block + size/sizeof(*block));
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
			uint32_t finalize = slab->finalize[i/32];
			uint32_t mask = 0x80000000;
			slab->finalize[i/32] = 0;
			for(; i<slab->type->count && mask; i++, mask>>=1) {
				if (finalize & mask) {
					slab->type->finalize(slab->data + slab->type->esize*i);
					debug_fillbuf(slab->data + slab->type->esize*i, slab->type->esize, 0x0f);
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
	slab_gc();

	if (gc_stats.inuse >= gc_stats.peak) {
		gc_stats.peak = gc_stats.inuse;
	}

	/* Finalize elements now available */
	slab_type_t * stype = types;
	while(stype) {
		slab_t * slab = stype->first;

		while(slab) {
#if 0
			if (stype->finalize) {
				slab_finalize(slab);
			}
#endif
			if (stype->finalize) {
				/* Step through each finalizable slot */
				int slot=bitarray_firstset(slab->finalize, stype->count);
				while(slot>=0) {
					slab->type->finalize(slab->data + slab->type->esize*slot);
					bitarray_set(slab->finalize, slot, 0);
					bitarray_set(slab->available, slot, 1);
					slot=bitarray_firstset(slab->finalize, stype->count);
				}
			} else {
				bitarray_or(slab->available, slab->finalize, stype->count);
			}

			/* Release page if now empty */
			if (slab_all_free(slab)) {
				slab_t * empty = slab;
				LIST_NEXT(stype->first, slab);
				LIST_DELETE(stype->first, empty);
				page_heap_free(empty);
			} else {
				LIST_NEXT(stype->first, slab);
			}
		}

		LIST_NEXT(types, stype);
	}
	arena_free(gcarena);
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
