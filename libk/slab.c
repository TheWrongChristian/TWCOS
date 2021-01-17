#include "slab.h"

#if INTERFACE
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

struct slab_type_t {
	uint32_t magic;
	size_t esize;
	size_t slotsize;
	int count;
	struct slab * first;
	slab_type_t * next, * prev;
	void (*mark)(void *);
	void (*finalize)(void *);
	mutex_t lock[1];
};

struct slab_weakref_t {
	void * p;
	int chances;
	unsigned int seq;
};

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

#define GCROOT __attribute__((section(".gcroot")))

#endif

exception_def OutOfMemoryException = { "OutOfMemoryException", &Exception };
exception_def AllocationTooBigException = { "AllocationTooBigException", &Exception };

typedef struct slab_slot_t slab_slot_t;
struct slab_slot_t {
	uint64_t seq;
#ifdef DEBUG
	char * file;
	int line;
	void * backtrace[9];
#endif
};

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

static slab_t * slab_new(slab_type_t * stype)
{
	if (0 == stype->magic) {
		/* Initialize type */
		stype->first = 0;
		stype->magic = 997 * (uint32_t)stype;
		stype->slotsize = ROUNDUP(stype->esize+sizeof(slab_slot_t), sizeof(intptr_t));

		/*           <-----------------d------------------>
		 * | slab_t |a|f|              data                |
		 *  <-----------------page size------------------->
		 * data + a + f = ARCH_PAGE_SIZE-sizeof(slab_t)
		 * c*s + (c+31)/8 + (c+31)/8 = psz-slab_t = d
		 * c*s + (c+31)/4 = psz-slab_t = d
		 * 4c*s + (c+31) = psz-slab_t = 4d
		 * 4c*s + c + 31 = psz-slab_t = 4d
		 * 4c*s + c = 4d - 31
		 * c(4s + 1) = 4d - 31
		 * c = (4d - 31) / (4s + 1)
		 */
		stype->count = (4*(ARCH_PAGE_SIZE-sizeof(slab_t))-31) / (4 * stype->slotsize + 1);
		slab_lock(0);
		LIST_APPEND(types, stype);
		slab_unlock();
	}

	/* Allocate and map page */
	slab_t * slab = page_heap_alloc();
	if (slab) {
		slab->magic = stype->magic;
		slab->type = stype;
		slab->available = (uint32_t*)(slab+1);
		slab->finalize = slab->available + (slab->type->count+32)/32;
		slab->data = (slab_slot_t*)(slab->finalize + (slab->type->count+32)/32);
		bitarray_setall(slab->available, stype->count, 1);

		LIST_PREPEND(stype->first, slab);

		assert(slab->type);
	}

	return slab;
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


#define SLAB_SLOT(slab, slot) ((slab_slot_t*)(slab->data + slab->type->slotsize*slot))
#define SLAB_SLOT_USER(slab, slot) (SLAB_SLOT(slab, slot)+1)

#if 1
#define SLAB_SLOT_NUM(slab, p) ((((char*)p)-slab->data) / slab->type->slotsize)
#else
static int SLAB_SLOT_NUM(slab_t * slab, void * p)
{
	char * user = p;
	ptrdiff_t diff = user - slab->data;
	int slot = diff / slab->type->slotsize;
	assert(slot<slab->type->count);
	return slot;
}
#endif


void * slab_alloc_p(slab_type_t * stype)
{
	static unsigned int seq = 0;
	mutex_lock(stype->lock);
	slab_t * slab = stype->first ? stype->first : slab_new(stype);

	slab_lock(0);
	while(slab) {
		assert(stype == slab->type);
		int slot = bitarray_firstset(slab->available, slab->type->count);
		assert(slot<slab->type->count);
		if (slot>=0) {
			if (bitarray_get(slab->finalize, slot)) {
				kernel_break();
			}
			bitarray_set(slab->available, slot, 0);
			//bitarray_set(slab->finalize, slot, 0);
			stype->first = slab;
			slab_unlock();
			mutex_unlock(stype->lock);
			slab_slot_t * entry = SLAB_SLOT(slab, slot);
			void * p = SLAB_SLOT_USER(slab, slot);
			assert(p==(void*)(entry+1));
			SLAB_SLOT(slab, slot)->seq = ++seq;
			debug_checkbuf(p, slab->type->esize);
			debug_fillbuf(p, slab->type->esize, 0x0a);
			return p;
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
#if 0
	/* Previous context */
	arena_state state;
	struct gccontext * prev;
#endif
} context[256];

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

	/* Put the roots in the queue */
	extern char gcroot_start[];
	extern char gcroot_end[];
	slab_gc_mark_range(gcroot_start, gcroot_end);
}

static slab_t * slab_get(void * p)
{
	if (arch_is_heap_pointer(p)) {
		/* Check magic numbers */
		slab_t * slab = ARCH_PAGE_ALIGN(p);

		if (slab == ARCH_PAGE_ALIGN(slab->data) && slab->magic == slab->type->magic && (slab_slot_t*)slab->data <= (slab_slot_t*)p) {
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
		int slot = SLAB_SLOT_NUM(slab, root);
		if (slot>=slab->type->count) {
			/* Not a valid pointer - don't mark */
			return;
		}
		assert(slot<slab->type->count);

		/* Adjust root to point to the start of the slab slot */
		root = SLAB_SLOT_USER(slab, slot);

		if (bitarray_get(slab->finalize, slot)) {
			/* Marked for finalization, clear the mark */
			bitarray_set(slab->finalize, slot, 0);
			gc_stats.inuse += slab->type->esize;

			if (slab->type->mark) {
				/* Call type specific mark */
				slab->type->mark(root);
			} else {
				/* Generic mark */
				slab_gc_mark_block(root, slab->type->esize);
			}
		}
	}
}

void slab_gc_mark_range(void * from, void * to)
{
	gclevel++;
	context[gclevel].from = from;
	context[gclevel].to = to;
}

void slab_gc()
{
	mutex_t lock[1] = {0};

	MUTEX_AUTOLOCK(lock) {
		while(gclevel) {
			/* Check the next pointer in the block */
			if (context[gclevel].from && context[gclevel].from < context[gclevel].to) {
				slab_gc_mark(*context[gclevel].from++);
			} else {
				context[gclevel].from = context[gclevel].to = 0;
				gclevel--;
			}
		}
	}
}

void slab_gc_mark_block(void ** block, size_t size)
{
	slab_gc_mark_range(block, block + size/sizeof(*block));
}

void slab_gc_end()
{
	if (gc_stats.inuse >= gc_stats.peak) {
		gc_stats.peak = gc_stats.inuse;
	}

	/* Finalize elements now available */
	slab_type_t * stype = types;
	while(stype) {
		slab_t * slab = stype->first;

		while(slab) {
			/* Step through each finalizable slot */
			int slot=bitarray_firstset(slab->finalize, stype->count);
			while(slot>=0) {
				slab_slot_t * entry = SLAB_SLOT(slab, slot);
				if (stype->finalize) {
					slab->type->finalize(SLAB_SLOT_USER(slab, slot));
				}
				debug_fillbuf(entry+1, slab->type->esize, 0xc0);
				entry->seq = 0;
				bitarray_set(slab->finalize, slot, 0);
				bitarray_set(slab->available, slot, 1);
				slot=bitarray_firstset(slab->finalize, stype->count);
			}

			/* Release page if now empty */
			if (0 && slab_all_free(slab)) {
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
	slab_unlock();
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

static void slab_weakref_mark(void * p)
{
	slab_weakref_t * ref = (slab_weakref_t*)p;
	if (ref->chances) {
		ref->chances--;
		slab_gc_mark(ref->p);
	}
}

slab_weakref_t * slab_weakref(void * p)
{
	static slab_type_t weakrefs[] = {
		SLAB_TYPE(sizeof(slab_weakref_t), slab_weakref_mark, 0),
	};
	static slab_weakref_t nullref[] = {0};

	slab_t * slab = slab_get(p);
	if (slab) {
		slab_weakref_t * ref = slab_alloc(weakrefs);
		int slot = SLAB_SLOT_NUM(slab, p);
		ref->seq = SLAB_SLOT(slab, slot)->seq;
		ref->p = p;
		ref->chances=0;

		return ref;
	}

	return nullref;
}

void * slab_weakref_get(slab_weakref_t * ref)
{
	slab_t * slab = slab_get(ref->p);
	if (slab) {
		int slot = SLAB_SLOT_NUM(slab, ref->p);
		if (ref->seq == SLAB_SLOT(slab, slot)->seq) {
			return ref->p;
		}
	}

	return 0;
}

#define SLAB_MAX_DATA_AREA(slots) ROUNDDOWN((((ARCH_PAGE_SIZE-sizeof(slab_t)-2*sizeof(uint32_t))/slots)-sizeof(slab_slot_t)),8)
static slab_type_t pools[] = {
	SLAB_TYPE(8, 0, 0),
	SLAB_TYPE(12, 0, 0),
	SLAB_TYPE(16, 0, 0),
	SLAB_TYPE(24, 0, 0),
	SLAB_TYPE(32, 0, 0),
	SLAB_TYPE(48, 0, 0),
	SLAB_TYPE(64, 0, 0),
	SLAB_TYPE(96, 0, 0),
	SLAB_TYPE(128, 0, 0),
	SLAB_TYPE(196, 0, 0),
	SLAB_TYPE(256, 0, 0),
	SLAB_TYPE(384, 0, 0),
	SLAB_TYPE(512, 0, 0),
	SLAB_TYPE(768, 0, 0),
	SLAB_TYPE(1024, 0, 0),
	SLAB_TYPE(SLAB_MAX_DATA_AREA(3), 0, 0),
	SLAB_TYPE(SLAB_MAX_DATA_AREA(2), 0, 0),
	SLAB_TYPE(SLAB_MAX_DATA_AREA(1), 0, 0),
};

#ifdef DEBUG

static slab_slot_t * audit[32];

void * add_alloc_audit(void * p, char * file, int line, size_t size, slab_type_t * type)
{
	static int next = 0;

	slab_slot_t * slot = ((slab_slot_t *)p)-1;
	audit[next] = slot;

	if (countof(audit) == ++next) {
		next = 0;
	}

#if DEBUG
	slot->file = file;
	slot->line = line;
	thread_backtrace(slot->backtrace, countof(slot->backtrace));
#endif

	return p;
}

#if 0
void dump_alloc_audit(void * p)
{
	for(int i=0; i<countof(audit); i++) {
		slab_slot_t * slot = audit[i];
		char * cp = (char*)p;
		char * base = (char*)slot+1;

		if (cp>=base && cp<base+slot->size) {
			kernel_printk("pointer alloc'd at %s:%d\n", slot->file, slot->line);
		}
	}
}
#endif

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
