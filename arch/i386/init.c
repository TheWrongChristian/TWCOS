#include <stddef.h>
#include <stdint.h>

#include <libk/libk.h>
#if 0
#include <kernel/kernel.h>
#endif
#include "init.h"


extern uint32_t pg_dir[1024];
extern uint32_t pt_00000000[1024];

BOOTSTRAP_CODE void bootstrap_paging_init()
{
	int i;
	pg_dir[0] = pg_dir[768] = ((uint32_t)pt_00000000) | 0x3;
	for(i=0; i<1024; i++) {
		pt_00000000[i] = (i * 4096) | 0x3;
	}
}

static char * mem_type(int type)
{
	switch(type) {
	case MULTIBOOT_MEMORY_AVAILABLE:
		return "Available";
	case MULTIBOOT_MEMORY_RESERVED:
		return "Reserved";
	case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
		return "ACPI Reclaimable";
	case MULTIBOOT_MEMORY_NVS:
		return "NVS";
	case MULTIBOOT_MEMORY_BADRAM:
		return "Bad";
	}
	return "Unknown";
}

extern char _bootstrap_start;
extern char _bootstrap_end;
extern char _bootstrap_nextalloc;
static char * nextalloc = &_bootstrap_nextalloc;

#define ALIGNMENT 16

void * bootstrap_alloc(size_t size)
{
        void * m = (void*)nextalloc;
        size += (ALIGNMENT-1);
        size &= (~(ALIGNMENT-1));
        nextalloc += size;

        return m;
}

static void bootstrap_finish()
{
	nextalloc += ARCH_PAGE_SIZE;
	nextalloc = (char*)((uint32_t)nextalloc & ~(ARCH_PAGE_SIZE-1));
}

void * arch_heap_page()
{
	void * p = nextalloc;
	nextalloc += ARCH_PAGE_SIZE;
	return p;
}

void arch_init()
{
	int i;
	ptrdiff_t koffset = &_bootstrap_nextalloc - &_bootstrap_end;
	page_t pstart;
	page_t pend;

	for(i=0;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			kernel_printk("Map %d -\t0x%x\t(%d)\t%s\n", i, (int)mmap->addr, (int)mmap->len, mem_type(mmap->type) );
			if (MULTIBOOT_MEMORY_AVAILABLE == mmap->type) {
				/*
				 * Add the memory to the pool of available
				 * memory.
				 */
				uint32_t page = mmap->addr >> 12;
				uint32_t count = mmap->len >> 12;

				page_add_range(page, count);
			}
		} else {
			break;
		}
	}

	pstart = ((uint32_t)&_bootstrap_start)>>12;
	pend = ((uint32_t)(nextalloc-koffset))>>12;
	for(i=0;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			if (MULTIBOOT_MEMORY_AVAILABLE == mmap->type) {
				/*
				 * Add the memory to the pool of available
				 * memory.
				 */
				uint32_t page = mmap->addr >> 12;
				uint32_t count = mmap->len >> 12;
				int i;

				for(i=0; i<count; i++, page++) {
					if (page<pstart || page > pend) {
						page_free(page);
					}
				}
			}
		} else {
			break;
		}
	}
	i386_init();
	vmap_init();
	pci_scan();
	bootstrap_finish();
	kernel_printk("Bootstrap end - 0x%p\n", nextalloc);
}

#if INTERFACE
#define BOOTSTRAP_DATA __attribute__((section(".bootstrap_data")))
#define BOOTSTRAP_CODE __attribute__((section(".bootstrap_code")))

#endif
