#include "vm.h"


/*
 * Virtual Memory manager
 *
 * VM is managed in segments. All referenced VM must be backed
 * by a segment (including kernel memory)
 *
 * Segments are backed by drivers, which can be:
 * - Anonymous - Page faults are serviced by fresh pages.
 * - Hardware - Page faults are serviced by mapping to underlying hardware.
 * - File - Page faults are serviced by the filesystem, for memory mapped files.
 * - Shadow segments - Page faults are serviced by an underlying segment. This
 *   forms the basis for memory sharing with either shared or copy on write
 *   semantics.
 *
 * Upon a page fault, the kernel resolves the faulting address to a segment
 * structure either in kernel address space (shared by all processes) or
 * process address space (private to each process).
 *
 * We then examine the page fault type and desired segment semantics to decide
 * whether to service the page fault and return to the faulting code, raise a
 * fault in the code (which might translate into a SIGSEGV in a UNIX application.)
 *
 *  Fault	| Seg		| Action
 * ------------------------------------------
 * Not present	| Any		| Map in by driver
 *
 * Permission	| Any		| Raise exception
 *
 * Read-only	| COW		| Create copy of existing mapping in new page
 *
 * Read-only	| Read-only	| Raise exception
 * 
 */

#if INTERFACE

#include <stddef.h>

#define SEGMENT_R 0x1
#define SEGMENT_W 0x2
#define SEGMENT_X 0x4
#define SEGMENT_U 0x8

typedef void (*segment_faulter)(struct segment_s * seg, void * p, int write, int user, int present);
typedef struct segment_s {
	segment_faulter fault;
	void * base;
	size_t size;
	int perms;
	vector_t * pages;
	struct segment_s * backing;

	/* Per segment data */
	union {
		struct {
		} file;
	};
} segment_t;
#endif

typedef struct segment_anonymous_s {
	segment_t segment;
}segment_anonymous_t;

static slab_type_t segments[1];
void vm_init()
{
	static int inited = 0;
	if (!inited) {
		inited = 1;
		slab_type_create(segments, sizeof(segment_t), 0, 0);
	}
}

static void vm_invalid_pointer(void * p, int write, int user, int present)
{
	kernel_panic("Invalid pointer: %p\n", p);
}

void vm_page_fault(void * p, int write, int user, int present)
{
	map_t * as = arch_get_thread()->as;
	thread_lock(as);
	segment_t * seg = map_get_le(as, p);
	thread_unlock(as);

	if (seg) {

		long offset = (char*)p - (char*)seg->base;

		if (offset < seg->size) {
			if (!present) {
				page_t page = page_alloc();
				vmap_map(as, p, page, SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
			} else if (write && SEGMENT_W & seg->perms) {
				page_t page = vmap_get_page(as, p);
				vmap_map(as, p, page, SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
			} else {
				vm_invalid_pointer(p, write, user, present);
			}
#if 0
			/* Defer to the segment driver */
			seg->fault(seg, p, write, user, present);
#endif
			return;
		}
	}
	/* Invalid pointer */
	vm_invalid_pointer(p, write, user, present);
}

static void vm_segment_anonymous_fault(segment_t * seg, void * p, int write, int user, int present)
{
	map_t * as = arch_get_thread()->as;
	long offset = (char*)p - (char*)seg->base;
	if (!present) {
		page_t page = page_alloc();
		vmap_map(as, p, page, SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
	}
}

segment_t * vm_segment_base( segment_faulter fault, void * p, size_t size, int perms, segment_t * backing)
{
	vm_init();
	segment_t * seg = slab_alloc(segments);

	seg->fault = fault;
	seg->base = p;
	seg->size = size;
	seg->perms = perms;
	seg->backing = backing;
	seg->pages = vector_new();

	return seg;
}

segment_t * vm_segment_anonymous(void * p, size_t size, int perms, segment_t * backing)
{
	segment_t * seg = vm_segment_base(vm_segment_anonymous_fault, p, size, perms, backing);

	return seg;
}

segment_t * vm_segment_copy(segment_t * from, int private)
{
	segment_t * seg = slab_alloc(segments);

	if (vm_segment_anonymous_fault == from->fault) {
		seg->fault = from->fault;
		seg->base = from->base;
		seg->size = from->size;
		seg->perms = from->perms;
		seg->backing = from->backing;
		seg->pages = vector_new();
	} else {
	}
}
