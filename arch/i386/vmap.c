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
extern uint32_t pg_dir[1024];
extern uint32_t pt_00000000[1024];
static __attribute__((section(".aligned"))) pte_t pgdirs[ASID_COUNT][1024];
static __attribute__((section(".aligned"))) pte_t pgktbl[1024];

#define VMAP_PAGE(add) ((page_t)add-(_kernel_offset-_kernel_offset_bootstrap) >> ARCH_PAGE_SIZE_LOG2)

/*
 * Page dirs are at the top of the address space
 */
static pte_t * pgtbls = (void *)(0xffffffff << (2 + ARCH_PAGE_TABLE_SIZE_LOG2 + ASID_COUNT_LOG2));

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

	if (0 == vid) {
		return pgtbls;
	}

	for(i=0; i<ASID_COUNT; i++) {
		if (vid == asids[i].vid) {
			asids[i].seq = seq;
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
	if (0 == vid) {
		return &pgdir[ARCH_PAGE_TABLE_SIZE-ASID_COUNT];
	}
	int i = (pgdir-pgtbls) >> ARCH_PAGE_TABLE_SIZE_LOG2;

	return &pgdir[ARCH_PAGE_TABLE_SIZE-ASID_COUNT+i];
}

void vmap_set_asid(asid vid)
{
	pte_t * pgdir = vmap_get_pgdir(vid);
	page_t pageno = *pgdir >> ARCH_PAGE_SIZE_LOG2;
	set_page_dir(pageno);
}

page_t vmap_get_page(asid vid, void * vaddress)
{
	page_t vpage = (uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2;
	pte_t * pgtbl = vmap_get_pgtable(vid);
	uint32_t pte = pgtbl[vpage];

	if (pte & 0x1) {
		return pte >> ARCH_PAGE_SIZE_LOG2;
	}
	return 0;
}

static void vmap_set_pte(asid vid, void * vaddress, pte_t pte)
{
	page_t vpage = (uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2;
	pte_t * pgtbl = vmap_get_pgtable(vid);

	if (0 == vmap_get_page(vid, pgtbls+vpage)) {
		page_t page = page_alloc();
		pte_t * pgtbl = pgtbls;

		for(int i=0; i<ASID_COUNT; i++, pgtbl += ARCH_PAGE_TABLE_SIZE) {
			vmap_map(0, pgtbl+vpage, page, 1, 0);
		}
	}
	pgtbl[vpage] = pte;
	invlpg(vaddress);
}

void vmap_map(asid vid, void * vaddress, page_t page, int rw, int user)
{
	/* Map the new page table entry */
	pte_t pte = page << ARCH_PAGE_SIZE_LOG2 | 0x1;
	if (rw) {
		pte |= 0x2;
	}
	if (user) {
		pte |= 0x4;
	}
	vmap_set_pte(vid, vaddress, pte);
}

void vmap_mapn(asid vid, int n, void * vaddress, page_t page, int rw, int user)
{
	int i = 0;
	char * vp = vaddress;

	for(i=0; i<n; i++, vp += ARCH_PAGE_SIZE) {
		vmap_map(vid, vp, page+i, rw, user);
	}
}

void vmap_unmap(asid vid, void * vaddress)
{
	vmap_set_pte(vid, vaddress, 0);
}

/*
 * Page fault handler
 */
void vmap_fault(void * p, int write, int user, int present)
{
	if ((char*)p > (char*)pgtbls) {
		/* Handle page tables specially */
	} else {
		/* Hand off to VM to resolve */
		vm_page_fault(p, write, user, present);
	}
}

static void vmap_test()
{
}

extern char code_start[];
extern char data_start[];

void vmap_init()
{
	INIT_ONCE();

	int d;
	int i;

	char * p = code_start;
	char * end = arch_heap_page();
	unsigned int offset = _kernel_offset-_kernel_offset_bootstrap;
	uint32_t pde = ((uint32_t)pgktbl)-offset | 0x3;
	uint32_t pgdirs_p = (uint32_t)((char*)pgdirs-offset);
	uint32_t pstart = (uint32_t)code_start - offset;
	uint32_t pend = (uint32_t)end - offset;

	/*
	 * Map page tables into VM
	 */
	for(d=1024-ASID_COUNT; d<1024; d++) {
		/* Bootstrap page table */
		pg_dir[d] = pgdirs_p | 0x3;
		/* Runtime page tables */
		for(i=0; i<ASID_COUNT; i++) {
			pgdirs[i][d] = pgdirs_p | 0x3;
		}
		pgdirs_p += ARCH_PAGE_SIZE;
	}
	for(d=pstart & 0xfffff000; d<pend;d+=ARCH_PAGE_SIZE) {
		pgktbl[d>>ARCH_PAGE_SIZE_LOG2] = d | 0x3;
	}

	/*
	 * Map kernel image directory into VM
	 */
	for(i=0; i<ASID_COUNT; i++) {
		pgdirs[i][(uint32_t)code_start >> (ARCH_PAGE_SIZE_LOG2+10)] = pde;
	}
	vmap_set_asid(0);
	for(; p<data_start; p+=ARCH_PAGE_SIZE) {
		vmap_map(0, p, (uint32_t)(p-offset) >> ARCH_PAGE_SIZE_LOG2, 0, 0);
	}
	for(; p<end; p+=ARCH_PAGE_SIZE) {
		vmap_map(0, p, (uint32_t)(p-offset) >> ARCH_PAGE_SIZE_LOG2, 1, 0);
	}
	vmap_test();
}


#if INTERFACE

#include <stdint.h>

typedef void * asid;
typedef uint32_t page_t;
typedef uint32_t pte_t;

#endif /* INTERFACE */
