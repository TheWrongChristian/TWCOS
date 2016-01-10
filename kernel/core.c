#include <arch/arch.h>

#include "core.h"



#if INTERFACE

#define page_to_ptr(page) (void*)((long)page * ARCH_PAGE_SIZE)

struct kernel_mmap {
	uint32_t pbase;
	int pages;
	int inuse;
	uint8_t * freemap;
}
#endif

/*
 * Up to 32 maps
 */
static struct kernel_mmap mmap[32];
