#include <stddef.h>
#include <stdint.h>
#include "vmap.h"

/*
 * Reserve address spaces
 */
#define ASID_COUNT_LOG2 2
#define ASID_COUNT (1<<ASID_COUNT_LOG2)
typedef pte_t pgtbls_t[1<<ARCH_PAGE_TABLE_SIZE_LOG2];

extern char _kernel_offset_bootstrap[0];
extern char _kernel_offset[0];
extern uint32_t pg_dir[1024];
extern uint32_t pt_00000000[1024];
static __attribute__((section(".aligned"))) pte_t pgdirs[ASID_COUNT][1024];
static __attribute__((section(".aligned"))) pte_t pgktbl[1024];

#define VMAP_PAGE(add) ((page_t)(add)-(_kernel_offset-_kernel_offset_bootstrap) >> ARCH_PAGE_SIZE_LOG2)

/*
 * Page dirs are at the top of the address space
 */
static pgtbls_t * pgtbls = (void *)((intptr_t)(-ASID_COUNT*sizeof(pgtbls_t)));


static struct {
	asid vid;
	int seq;
} asids[1 << ASID_COUNT_LOG2];

static void vmap_pgtbl_init(int ptid)
{
	ptrdiff_t poffset = (uint32_t)(_bootstrap_nextalloc - _bootstrap_end) >> ARCH_PAGE_SIZE_LOG2;
	pte_t * pgtbl=pgtbls[ptid];
	pte_t * pgdir=pgdirs+ptid;
	for(int i=0; i<poffset; i+=1024) {
		if (pgdir[i>>(ARCH_PAGE_SIZE_LOG2-2)]) {
			for(int j=0; j<1024; j++) {
				pgtbl[i+j] = 0;
			}
		}
	}
}

static int ptidseq = 0;
static int vmap_probe_ptid(asid vid)
{
	if (0 == vid || kas == vid) {
		return 0;
	}

	for(int i=0; i<ASID_COUNT; i++) {
		if (vid == asids[i].vid) {
			asids[i].seq = ptidseq;
			return i;
		}
	}

	return -1;
}

static int vmap_get_ptid(asid vid)
{
	int ptid = vmap_probe_ptid(vid);

	if (ptid>=0) {
		return ptid;
	}

	/* Not found - Allocate a new asid */
	int lowseq = INT32_MAX;
	ptid = 0;
	for(int i=0; i<ASID_COUNT; i++) {
		if (lowseq > asids[i].seq) {
			lowseq = asids[i].seq;
			ptid = i;
		}
	}

	asids[ptid].vid = vid;
	asids[ptid].seq = ++ptidseq;
	vmap_pgtbl_init(ptid);

	return ptid;
}

static pte_t * vmap_get_pgtable(asid vid)
{
	int ptid = vmap_get_ptid(vid);

	return pgtbls[ptid];
}

static mutex_t lock[] = {0};

void vmap_set_asid(asid vid)
{
	MUTEX_AUTOLOCK(lock) {
		int ptid = vmap_get_ptid(vid);

		set_page_dir(VMAP_PAGE(pgdirs[ptid]));
	}
}

void vmap_release_asid(asid vid)
{
	MUTEX_AUTOLOCK(lock) {
		int ptid = vmap_probe_ptid(vid);

		if (ptid>=0) {
			asids[ptid].vid = 0;
		}
	}
}

page_t vmap_get_page(asid vid, void * vaddress)
{
	uint32_t pte = 0;

	MUTEX_AUTOLOCK(lock) {
		page_t vpage = (uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2;
		pte_t * pgtbl = vmap_get_pgtable(vid);
		pte = pgtbl[vpage];
	}

	if (pte & 0x1) {
		return pte >> ARCH_PAGE_SIZE_LOG2;
	}
	return 0;
}

static pte_t vmap_get_pte(asid vid, void * vaddress)
{
	pte_t pte = 0;

	MUTEX_AUTOLOCK(lock) {
		int ptid = vmap_probe_ptid(vid);
		if (ptid>=0) {
			page_t vpage = (uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2;
			pte_t * pgtbl = pgtbls[ptid];
			int pgdirnum = vpage >> 10;

			/* Check if we have a page directory */
			if (pgdirs[ptid][pgdirnum] & 1){
				/* Get the page table entry */
				pte = pgtbl[vpage];
			}
		}
	}

	return pte;
}

static void vmap_set_pte(asid vid, void * vaddress, pte_t pte)
{
	MUTEX_AUTOLOCK(lock) {
		int ptid = vmap_get_ptid(vid);
		page_t vpage = (uint32_t)vaddress >> ARCH_PAGE_SIZE_LOG2;
		pte_t * pgtbl = pgtbls[ptid];
		int pgdirnum = vpage >> 10;

		if (0 == (pgdirs[ptid][pgdirnum] & 1)){
			/* No page table, create a new one */
			ptrdiff_t koffset = (uint32_t)(_bootstrap_nextalloc - _bootstrap_end);

			/* page_alloc might end up needing lock */
			page_t page = page_alloc(0);

			if (((uintptr_t)vaddress)<koffset) {
				/* User mapping, just this ASID */
				pgdirs[ptid][pgdirnum] = (page << ARCH_PAGE_SIZE_LOG2) | 7;
			} else {
				/* Kernel mapping, reflect across all ASID */
				for(int i=0; i<ASID_COUNT; i++) {
					pgdirs[i][pgdirnum] = (page << ARCH_PAGE_SIZE_LOG2) | 3;
				}
			}

			/* Clean directory */
			memset(pgtbl + (vpage&~0x3ff), 0, ARCH_PAGE_SIZE);
		}
		pgtbl[vpage] = pte;
		/* FIXME: Only need this if vid is current or kernel as */
		invlpg(vaddress);
	}
}

void vmap_map(asid vid, void * vaddress, page_t page, int rw, int user)
{
	assert(page);

	/* Map the new page table entry */
	pte_t pte = page << ARCH_PAGE_SIZE_LOG2 | 0x1;
	if (rw) {
		pte |= 0x2;
	}
	if (user) {
		pte |= 0x4;
	}
	vmap_set_pte(vid, vaddress, pte);

	kernel_printk("[%x,%p] -> %d %s%s\n", (int)vid, vaddress, (int)page, user ? "u" : "", rw ? "rw" : "ro");
}

void vmap_mapn(asid vid, int n, void * vaddress, page_t page, int rw, int user)
{
	int i = 0;
	char * vp = vaddress;

	for(i=0; i<n; i++, vp += ARCH_PAGE_SIZE) {
		vmap_map(vid, vp, page+i, rw, user);
	}
}

int vmap_ismapped(asid vid, void * vaddress)
{
	return vmap_get_pte(vid, vaddress) & 0x1;
}

int vmap_iswriteable(asid vid, void * vaddress)
{
	return 0x3 == (vmap_get_pte(vid, vaddress) & 0x3);
}

int vmap_isuser(asid vid, void * vaddress)
{
	return 0x5 == (vmap_get_pte(vid, vaddress) & 0x5);
}

void vmap_unmap(asid vid, void * vaddress)
{
	vmap_set_pte(vid, vaddress, 0);
}

extern char code_start[];
extern char data_start[];

void vmap_init()
{
	INIT_ONCE();

	int d;
	int i;

	char * p = code_start;
	char * end = bootstrap_alloc(0);
	unsigned int offset = _kernel_offset-_kernel_offset_bootstrap;
	uint32_t pde = (((uint32_t)pgktbl)-offset) | 0x3;
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
}


#if INTERFACE

#include <stdint.h>

typedef void * asid;
typedef uint32_t page_t;
typedef uint32_t pte_t;

#endif /* INTERFACE */
