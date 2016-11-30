#include <stddef.h>
#include <stdint.h>

#include "core.h"

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
	uint32_t * available;
};

static int mmap_count = 0;
static struct kernel_mmap mmap[32];

void page_add_range(page_t base, uint32_t count)
{
	size_t bitmapsize = (count+31)/32;
	int i;
	mmap[mmap_count].base = base;
	mmap[mmap_count].count = count;
	mmap[mmap_count].free = 0;
	mmap[mmap_count].available = bootstrap_alloc(bitmapsize);
	for(i=0; i<bitmapsize; i++) {
		mmap[mmap_count].available[i] = 0;
	}
	mmap_count++;
}

void page_free(page_t page)
{
	int i = 0;
	for(; i<mmap_count; i++) {
		int p = page - mmap[i].base;
		if (p > 0 && p < mmap[i].count) {
			/*
			 * Page is in this memory range, mark it as free
			 */
			mmap[i].available[p/32] |= (0x80000000 >> p%32);
			mmap[i].free++;
			return;
		}
	}
	/* FIXME: Panic here */
}

page_t page_alloc()
{
	int m = mmap_count - 1;

	for(;m>=0; m--) {
		if (mmap[m].free>0) {
			int p;

			for(p=0; p<mmap[m].count; p+=32) {
				if (mmap[m].available[p/32]) {
					uint32_t mask = 0x80000000;
					for(; mask; mask >>= 1, p++) {
						if (mmap[m].available[p/32] & mask) {
							mmap[m].available[p/32] &= (~mask);
							mmap[m].free--;
							return mmap[m].base + p;
						}
					}
					/* FIXME: panic */
				}
			}
			/* FIXME: panic, we should have found a free page */
		}
	}
	/* FIXME: Out of memory panic or exception */
	return 0;
}

void * page_valloc()
{
	page_t page = page_alloc();
	void * p = arch_heap_page();
	vmap_map(0, p, page, 1, 0);

	return p;
}
