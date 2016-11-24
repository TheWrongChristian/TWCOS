#include <stddef.h>
#include <stdint.h>
#include "vmap.h"

/*
 * Reserve 8 address spaces
 */
#define ASID_COUNT_LOG2 3
#define ASID_COUNT (1<<ASID_COUNT_LOG2)

extern char _kernel_offset_bootstrap[0];
extern char _kernel_offset[0];
static __attribute__((section(".aligned"))) pte_t pgdirs[ASID_COUNT][1024];

#define VMAP_PAGE(add) ((char*)add - (&_kernel_offset-&_kernel_offset_bootstrap))

/*
 * Page dirs are at the top of the address space
 */
static pte_t * pgtbls = (void *)(0xffffffff << (ARCH_PAGE_TABLE_SIZE_LOG2 + ASID_COUNT_LOG2));

#if 0
static asid asids[1 << ASID_COUNT_LOG2];
#endif
static struct {
	asid vid;
	int seq;
} asids[1 << ASID_COUNT_LOG2];

static pte_t * vmap_get_pgtable(asid vid)
{
	int i;
	int lowseq = INT32_MAX;
	int lowi;
	static int seq = 0;

	for(i=0; i<ASID_COUNT; i++) {
		if (vid == asids[i].vid) {
			return pgtbls+ARCH_PAGE_TABLE_SIZE*i;
		}
	}

	/* Not found - Allocate a new asid */
	for(i=0; i<ASID_COUNT; i++) {
		if (lowseq > asids[i].seq) {
			lowseq = asids[i].seq;
			lowi = i;
		}
	}

	asids[lowi].vid = vid;
	asids[lowi].seq = ++seq;

	return pgtbls+ARCH_PAGE_TABLE_SIZE*lowi;
}

pte_t * vmap_get_pgdir(asid vid)
{
	pte_t * pgdir = vmap_get_pgtable(vid);
	int i = (pgdir-pgtbls) >> ARCH_PAGE_TABLE_SIZE_LOG2;

	return &pgdir[ARCH_PAGE_TABLE_SIZE-ASID_COUNT+i];
}

void vmap_set_asid(asid vid)
{
	pte_t * pgdir = vmap_get_pgdir(vid);
	page_t pageno = *pgdir >> ARCH_PAGE_SIZE_LOG2;
	set_page_dir(pageno);
}

void vmap_map(asid vid, void * vaddress, page_t page, int rw, int user)
{
	if (vid) {
		pte_t * pgtbl = vmap_get_pgtable(vid);
		uint32_t pte = page | 0x1;
		if (rw) {
			pte |= 0x2;
		}
		if (user) {
			pte |= 0x4;
		}
		pgtbl[(uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2] = pte;
	} else {
		int i = 0;
		pte_t * pgtbl = pgtbls;

		for(; i<ASID_COUNT; i++, pgtbl += ARCH_PAGE_TABLE_SIZE) {
			uint32_t pte = page | 0x1;
			if (rw) {
				pte |= 0x2;
			}
			if (user) {
				pte |= 0x4;
			}
			pgtbl[(uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2] = pte;
		}
	}
}

void vmap_unmap(asid vid, void * vaddress)
{
}

/*
 * Page fault handler
 */
void vmap_fault(void * p, int rw, int user)
{
}

static void vmap_test()
{
}

void vmap_init()
{
	int d;
	uint32_t pgdirs_p = (uint32_t)((char*)pgdirs-(_kernel_offset-_kernel_offset_bootstrap));
	arch_heap_page();
	for(d=1024-ASID_COUNT; d<1024; d++) {
		int i;
		for(i=0; i<ASID_COUNT; i++) {
			pgdirs[i][d] = pgdirs_p | 0x2 | 0x1;
		}
		pgdirs_p += ARCH_PAGE_SIZE;
	}
	vmap_test();
}


#if INTERFACE

#include <stdint.h>

typedef void * asid;
typedef uint32_t page_t;
typedef uint32_t pte_t;

/*
 * Sizes
 */
#define ARCH_PAGE_SIZE_LOG2 12
#define ARCH_PAGE_SIZE (1<<ARCH_PAGE_SIZE_LOG2)
#define ARCH_PAGE_TABLE_SIZE_LOG2 20
#define ARCH_PAGE_TABLE_SIZE (1<<ARCH_PAGE_TABLE_SIZE_LOG2)

#endif /* INTERFACE */
