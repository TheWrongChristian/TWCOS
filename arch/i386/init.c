#include <libk/libk.h>

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

extern int _bootstrap_end[1024][1];
void arch_init(struct stream * stream)
{
	int i = 0;
	for(;;i++) {
		multiboot_memory_map_t * mmap = multiboot_mmap(i);

		if (mmap) {
			stream_printf(stream, "Map %d - 0x%x (%d) %s\n", i, (int)mmap->addr, (int)mmap->len, mem_type(mmap->type) );
		} else {
			break;
		}
	}
	stream_printf(stream, "Bootstrap end - 0x%p\n", _bootstrap_end);
}

#if INTERFACE
#define BOOTSTRAP_DATA __attribute__((section(".bootstrap_data")))
#define BOOTSTRAP_CODE __attribute__((section(".bootstrap_code")))

#endif
