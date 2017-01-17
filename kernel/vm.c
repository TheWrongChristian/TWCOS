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
	void (*fault)(void * p, int write, int user, int present);
} segment_ops_t;

typedef struct segment_s {
	segment_ops_t * ops;
	struct segment_s * backing;
} segment_t;

static slab_type_t segments[1];
void vm_init()
{
	slab_type_create(segments, sizeof(segment_t), 0, 0);
}

void vm_page_fault(void * p, int write, int user, int present)
{
	map_t * as = arch_get_thread()->as;
	segment_t * seg = map_get(as, p);

	if (0 == seg) {
		/* Invalid pointer */
		vm_invalid_pointer(p);

		/* Not reached */
		return;
	} else {
		/* Defer to the segment driver */
		seg->ops->fault(p, write, user, present);
	}
}

segment_t * vm_segment_anonymous( void * p, size_t size, int writeable, segment_t * backing)
{
	map_t * as = arch_get_thread()->as;
	segment_t * seg = slab_valloc(segments);

	map_put(as, p, seg);

	return seg;
}
