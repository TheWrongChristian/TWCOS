#include <stddef.h>
#include <stdint.h>

#include "core.h"

#if INTERFACE

#define CORE_ISADMA	(1<<0)
#define CORE_SUB4G	(1<<1)

#endif

/*
 * Page usage record
 */
struct page_usage {
	asid as;
	void * rmap;
};

/*
 * Up to 32 maps
 */
struct kernel_mmap {
	page_t base;
	int count;
	int free;
	int flags;
	uint32_t * available;
	uint32_t * finalize;
};

static spin_t mmap_lock[] = {0};
static int mmap_count = 0;
static struct kernel_mmap mmap[32];

int page_count()
{
	int count = 0;

	for(int i=0; i<mmap_count; i++) {
		count += mmap[i].count;
	}

	return count;
}

int page_count_free()
{
	int free = 0;

	for(int i=0; i<mmap_count; i++) {
		free += mmap[i].free;
	}

	return free;
}

void page_gc_begin()
{
}

void page_gc_mark(page_t page)
{
	if (0 == page) {
		/* Ignore page zero */
		return;
	}

	spin_lock(mmap_lock);
	for(int i=0; i<mmap_count; i++) {
		int p = page - mmap[i].base;
		if (p >= 0 && p < mmap[i].count) {
			/*
			 * Page is in this memory range, mark it as free
			 */
			if (mmap[i].available[p/32] & (0x80000000 >> p%32)) {
				kernel_panic("Page marked free is in use: %d", page);
			}
		}
	}
	spin_unlock(mmap_lock);
}

void page_gc_end()
{
}

void page_add_range(page_t base, uint32_t count, int flags)
{
	size_t bitmapsize = (count+31)/32;
	int i;
	mmap[mmap_count].base = base;
	mmap[mmap_count].count = count;
	mmap[mmap_count].free = 0;
	mmap[mmap_count].flags = flags;
	mmap[mmap_count].available = bootstrap_alloc(sizeof(mmap[mmap_count].available[0]) * bitmapsize);
	mmap[mmap_count].finalize = bootstrap_alloc(sizeof(mmap[mmap_count].finalize[0]) * bitmapsize);
	for(i=0; i<bitmapsize; i++) {
		mmap[mmap_count].available[i] = 0;
	}
	mmap_count++;
}

void page_free_all()
{
	for(int i=0; i<mmap_count; i++) {
		bitarray_setall(mmap[i].available, mmap[i].count, 1);
		mmap[i].free = mmap[i].count;
	}
}

void page_reserve(page_t page)
{
	for(int i=0; i<mmap_count; i++) {
		int p = page - mmap[i].base;
		if (p >= 0 && p < mmap[i].count) {
			bitarray_set(mmap[i].available, p, 0);
			mmap[i].free--;
			return;
		}
	}
}

monitor_t cleansignal[] = {0};
monitor_t freesignal[] = {0};
void page_free(page_t page)
{
	int i = 0;

	if (0 == page) {
		/* Ignore page zero */
		return;
	}

	spin_lock(mmap_lock);
	for(; i<mmap_count; i++) {
		int p = page - mmap[i].base;
		if (p >= 0 && p < mmap[i].count) {
			/*
			 * Page is in this memory range, mark it as free
			 */
			bitarray_set(mmap[i].available, p, 1);
			mmap[i].free++;
			break;
		}
	}
	spin_unlock(mmap_lock);
}

static page_t page_alloc_internal(int reserve, int flags)
{
	int m = mmap_count - 1;

	spin_lock(mmap_lock);
	if (page_count_free()<reserve) {
		spin_unlock(mmap_lock);
		return 0;
	}
	for(;m>=0; m--) {
		if (mmap[m].free>0) {
			if (flags && 0 == (mmap[m].flags & flags)) {
				continue;
			}
			int p = bitarray_firstset(mmap[m].available, mmap[m].count);
			if (p>=0) {
				bitarray_set(mmap[m].available, p, 0);
				mmap[m].free--;
				spin_unlock(mmap_lock);
				return mmap[m].base + p;
			}
		}
	}
	spin_unlock(mmap_lock);

	return 0;
}

page_t page_alloc(int flags)
{
	page_t page;
	static int reserve = 8;
	int reservesave = reserve;

	while(0 == (page = page_alloc_internal(reserve, flags))) {
		static monitor_t cleanlock[1] = {0};
		MONITOR_AUTOLOCK(cleanlock) {
			if (reserve) {
				reserve = 0;
				thread_gc();
				monitor_broadcast(cleanlock);
			} else {
				kernel_panic("Out of memory, including reserve");
			}
		}
	}
	reserve = reservesave;

	return page;
}

page_t page_calloc(int flags)
{
	page_t page = page_alloc(flags);
	page_clean(page);

	return page;
}

void page_clean(page_t page)
{
	static void * p = 0;
	static mutex_t lock[] = {0};

	MUTEX_AUTOLOCK(lock) {
		if (0==p) {
			p = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
		}

		vmap_map(0, p, page, 1, 0);
		memset(p, 0, ARCH_PAGE_SIZE);
	}
}

segment_t * heap;
static spin_t heap_cache_lock = {0};
static void ** heap_cache;
void * page_heap_alloc()
{
	void * p = 0;
	while(0==p) {
		SPIN_AUTOLOCK(&heap_cache_lock) {
			if (heap_cache) {
				p = heap_cache;
				heap_cache = heap_cache[0];
			}
		}

		if (0 == p) {
			p = arch_heap_page();
			if (0 == p) {
				thread_gc();
				continue;
			}
			page_t page = page_alloc(0);
			vmap_map(0, p, page, 1, 0);
			vmpage_t vmpage = {page: page};
			vmobject_put_page(heap->dirty, (char*)p - (char*)heap->base, &vmpage);
		}
	}

	memset(p, 0, ARCH_PAGE_SIZE);

	return p;
}

void page_heap_free(void * p)
{
	void ** pp = (void**)p;

	SPIN_AUTOLOCK(&heap_cache_lock) {
		pp[0] = heap_cache;
		heap_cache = pp;
	}
}
