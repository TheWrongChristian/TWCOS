#include <stddef.h>
#include <stdint.h>

#include "init.h"


extern uint32_t pg_dir[1024];
extern uint32_t pt_00000000[1024];
extern char _kernel_offset_bootstrap[0];
extern char _kernel_offset[0];

BOOTSTRAP_CODE void bootstrap_paging_init()
{
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
        nextalloc = PTR_ALIGN_NEXT(nextalloc,ALIGNMENT);
	void * m = nextalloc;
        nextalloc += size;

        return m;
}

static void bootstrap_finish()
{
        nextalloc = PTR_ALIGN_NEXT(nextalloc,ARCH_PAGE_SIZE);
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

void * modules[8]={0};
size_t modulesizes[8]={0};
void * initrd=0;
size_t initrdsize=0;


void arch_init()
{
	multiboot_info_t info[]={0};

	int i;
	ptrdiff_t koffset = _bootstrap_nextalloc - _bootstrap_end;
	int pcount = 0;

	/* Copy for reference */
	multiboot_copy(info);

	memset(zero_start, 0, zero_end-zero_start);

	/* Any multi-boot modules */
	for(i=0; i<countof(modules); i++) {
		multiboot_module_t * mod = multiboot_mod(i);
		if (mod) {
			modules[i] = (void*)(mod->mod_start+koffset);
			modulesizes[i] = mod->mod_end-mod->mod_start;
			nextalloc = koffset + mod->mod_end;
			nextalloc = PTR_ALIGN_NEXT(nextalloc,ARCH_PAGE_SIZE);
		} else {
			break;
		}
	}

	cli();
	initrd = modules[0];
	initrdsize = modulesizes[0];

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

				/* Only handle page ranges under 4GB */
				if (page+count<1<<20) {
					page_add_range(page, count, CORE_SUB4G);
					pcount += count;
				}
			}
		} else {
			break;
		}
	}

	/* 64MB max heap by default */
	heapend = ARCH_PAGE_ALIGN(data_start + (64<<20));
	vm_kas_start(heapend);

	vmobject_t * heapobject = vm_object_heap(nextalloc, heapend);
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
	heap = vm_segment_heap(nextalloc, heapobject);
	vm_init();
	page_t code_page = ((uintptr_t)code_start - koffset) >> ARCH_PAGE_SIZE_LOG2;
	page_t data_page = ((uintptr_t)data_start - koffset) >> ARCH_PAGE_SIZE_LOG2;
	vm_kas_add(vm_segment_direct(code_start, data_start - code_start, SEGMENT_R | SEGMENT_X, code_page ));
	vm_kas_add(vm_segment_direct(data_start, nextalloc - data_start, SEGMENT_R | SEGMENT_W, data_page ));
	vm_kas_add(heap);
	pci_scan(pci_probe_print);

	kernel_printk("Bootstrap end - %p\n", nextalloc);
	sti(0xffffffff);

	/* Initialize the console */
	console_initialize(info);

	kernel_startlogging(0);
}

#if INTERFACE
#define BOOTSTRAP_DATA __attribute__((section(".bootstrap_data")))
#define BOOTSTRAP_CODE __attribute__((section(".bootstrap_code")))

#endif
