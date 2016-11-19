#include "vmap.h"

/*
 * Reserve 8 address spaces
 */
#define ASID_COUNT_LOG2 3
#define ASID_COUNT (1<<ASID_COUNT_LOG2)

/*
 * Page size
 */
#define PAGE_SIZE_LOG2 12
#define PAGE_SIZE (1<<PAGE_SIZE_LOG2)

typedef uint32_t pgmap_t[1024];

/*
 * Page dirs are at the top of the address space
 */
static pgmap_t * pgdirs = (pgmap_t *)(0xffffffff << (PAGE_SIZE_LOG2 + ASID_COUNT_LOG2));

static asid asids[1 << ASID_COUNT_LOG2];

void vmap_set_asid(asid id)
{
}

void vmap_map(asid id, void * vaddress, page_t page)
{
}

void vmap_unmap(asid id, void * vaddress)
{
}

void vmap_init()
{
	int i;
	for(i=0; i<ASID_COUNT; i++) {
	}
}


#if INTERFACE

#include <stdint.h>

typedef int asid;
typedef uint32_t page_t;

#endif /* INTERFACE */
