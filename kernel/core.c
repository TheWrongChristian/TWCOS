#include <stddef.h>

#include "core.h"

extern char _bootstrap_nextalloc;
static char * nextalloc = &_bootstrap_nextalloc;

#define ALIGNMENT 16

static void * localalloc(size_t size)
{
	void * m = (void*)nextalloc;
	size += (ALIGNMENT-1);
	size &= (~(ALIGNMENT-1));
	nextalloc += size;
	arch_bootstrap_nextalloc(nextalloc);

	return m;
}

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
	struct page_usage * map;
};
static int mmap_count = 0;
static struct kernel_mmap mmap[32];

void page_add_range(page_t base, uint32_t count)
{
	mmap[mmap_count].base = base;
	mmap[mmap_count].count = count;
	mmap[mmap_count].free = 0;
	mmap[mmap_count].map = localalloc(sizeof(*mmap[mmap_count].map) * count);
	mmap_count++;
}

#if INTERFACE

#define page_to_ptr(page) (void*)((long)page * ARCH_PAGE_SIZE)

#endif

