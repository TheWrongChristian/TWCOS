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
typedef struct segment_ops_s {
	void (*fault)(segment_t * seg, void * p, int write, int user, int present);
} segment_ops_t;

#if INTERFACE

#include <stddef.h>

typedef struct segment_s {
	struct segment_ops_s * ops;
	void * base;
	size_t size;
	int writeable;
	int user;
	struct segment_s * backing;
	void * private;
} segment_t;
#endif

typedef struct segment_anonymous_s {
	segment_t segment;
}segment_anonymous_t;

static slab_type_t segment_anonymous[1];
void vm_init()
{
	slab_type_create(segment_anonymous, sizeof(segment_anonymous_t), 0, 0);
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
			/* Defer to the segment driver */
			seg->ops->fault(seg, p, write, user, present);
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
	segment_anonymous_t * aseg = seg->private;
	if (!present) {
		page_t page = page_alloc();
		vmap_map(as, p, seg->writeable, seg->user);
	}
}

segment_t * vm_segment_anonymous( void * p, size_t size, int writeable, segment_t * backing)
{
	static segment_ops_t anon_ops = {
		fault: vm_segment_anonymous_fault
	};
	segment_anonymous_t * seg = slab_alloc(segment_anonymous);

	seg->segment.private = seg;
	seg->base = p;
	seg->size = size;
	seg->ops = &anon_ops;
	seg->backing = 0;

	return &seg->segment;
}
