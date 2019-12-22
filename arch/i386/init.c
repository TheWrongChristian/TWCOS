#include <stddef.h>
#include <stdint.h>

#include "init.h"


extern uint32_t pg_dir[1024];
extern uint32_t pt_00000000[1024];
extern char _kernel_offset_bootstrap[0];
extern char _kernel_offset[0];

BOOTSTRAP_CODE void bootstrap_paging_init()
{
	INIT_ONCE();

	int i;
	uint32_t offset = _kernel_offset-_kernel_offset_bootstrap;
	pg_dir[0] = pg_dir[offset >> 22] = ((uint32_t)pt_00000000) | 0x3;
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

extern char _bootstrap_start[];
extern char _bootstrap_end[];
extern char _bootstrap_nextalloc[];
static char * nextalloc = _bootstrap_nextalloc;
static char * heapend;
extern char zero_start[];
extern char zero_end[];

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
	if (nextalloc<heapend) {
		void * p = nextalloc;
		nextalloc += ARCH_PAGE_SIZE;
		return p;
	} else {
		kernel_printk("Heap exhausted!");
		return 0;
	}
}

int arch_is_heap_pointer(void *p)
{
	return ((char*)p)>=&_bootstrap_nextalloc && (char*)p<nextalloc;
}

void * initrd=0;
size_t initrdsize=0;

void arch_init()
{
	INIT_ONCE();

	int i;
	ptrdiff_t koffset = _bootstrap_nextalloc - _bootstrap_end;
	void * modstart = 0;
	size_t modsize = 0;
	int pcount = 0;

	multiboot_module_t * mod = multiboot_mod(0);
	if (mod) {
		modstart = (void*)(mod->mod_start+koffset);
		modsize = mod->mod_end-mod->mod_start;
		nextalloc = koffset + mod->mod_end;
		nextalloc += ARCH_PAGE_SIZE;
		nextalloc = (char*)((uint32_t)nextalloc & ~(ARCH_PAGE_SIZE-1));
	}

	memset(zero_start, 0, zero_end-zero_start);
	cli();
	initrd = modstart;
	initrdsize = modsize;

	for(i=0;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			if (MULTIBOOT_MEMORY_AVAILABLE == mmap->type) {
				/*
				 * Add the memory to the pool of available
				 * memory.
				 */
				uint32_t page = mmap->addr >> ARCH_PAGE_SIZE_LOG2;
				uint32_t count = mmap->len >> ARCH_PAGE_SIZE_LOG2;

				page_add_range(page, count);
				pcount += count;
			}
		} else {
			break;
		}
	}

	/* 64MB max heap by default */
	heapend = ARCH_PAGE_ALIGN(data_start + (64<<20));
	vm_kas_start(heapend);

	vmobject_t * heapobject = vm_object_heap(pcount);
	i386_init();
	vmap_init();
	bootstrap_finish();

	page_t pstart = ((uint32_t)&_bootstrap_start)>>ARCH_PAGE_SIZE_LOG2;
	page_t pend = ((uint32_t)(nextalloc-koffset))>>ARCH_PAGE_SIZE_LOG2;
	page_free_all();
	page_reserve(0);
	for(page_t p=pstart; p<pend; p++) {
		page_reserve(p);
	}
#if 0
	for(i=0;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			if (MULTIBOOT_MEMORY_AVAILABLE == mmap->type) {
				/*
				 * Add the memory to the pool of available
				 * memory.
				 */
				uint32_t page = mmap->addr >> ARCH_PAGE_SIZE_LOG2;
				uint32_t count = mmap->len >> ARCH_PAGE_SIZE_LOG2;
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
#endif
	heap = vm_segment_heap(nextalloc, heapobject);
	vm_init();
	page_t code_page = ((uintptr_t)code_start - koffset) >> ARCH_PAGE_SIZE_LOG2;
	page_t data_page = ((uintptr_t)data_start - koffset) >> ARCH_PAGE_SIZE_LOG2;
	vm_kas_add(vm_segment_direct(code_start, data_start - code_start, SEGMENT_R | SEGMENT_X, code_page ));
	vm_kas_add(vm_segment_direct(data_start, nextalloc - data_start, SEGMENT_R | SEGMENT_W, data_page ));
	vm_kas_add(heap);
	pci_scan();

#if 0
	vmap_mapn(0, 0xa0, (char*)koffset, 0, 0, 0);
	vmap_map(0, (void*)0x100000, 0x100, 0, 0);
	for(i=0;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			kernel_printk("Map %d -\t0x%x\t(%d)\t%s\n", i, (int)mmap->addr, (int)mmap->len, mem_type(mmap->type) );
		} else {
			break;
		}
	}
#endif
	kernel_printk("Bootstrap end - %p\n", nextalloc);
	sti();
	kernel_startlogging(1);

#if 0
	address_info_t info[1];
	vm_resolve_address(kas, info);
	kasvmpage = vmobject_get_page(info->seg->dirty, info->offset);
#endif
}

#if INTERFACE
#define BOOTSTRAP_DATA __attribute__((section(".bootstrap_data")))
#define BOOTSTRAP_CODE __attribute__((section(".bootstrap_code")))

#endif
