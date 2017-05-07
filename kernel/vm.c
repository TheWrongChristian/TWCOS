#include "vm.h"


/*
 * Virtual Memory manager
 *
 * VM is managed in segments. All referenced VM must be backed
 * by a segment (including kernel memory)
 *
 * Segments are backed by objects, which can be:
 * - Anonymous - Page faults are serviced by fresh pages.
 * - Direct - Page faults are serviced by mapping to underlying hardware.
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
#define SEGMENT_P 0x10

enum object_type_e { OBJECT_DIRECT, OBJECT_ANON, OBJECT_FILE };

typedef struct vmobject_ops_s {
	page_t (*get_page)(struct vmobject_s * object, int offset);
	void (*put_page)(struct vmobject_s * object, int offset, page_t page);
} vmobject_ops_t;

typedef struct vmobject_s {
	vmobject_ops_t * ops;
	/* Per object type data */
	int type;
	union {
		struct {
			page_t base;
			size_t size;
		} direct;
		struct {
			vector_t * pages;
		} anon;
		struct {
		} file;
	};
} vmobject_t;

typedef struct anon_page_s {
	int ref;
	page_t page;
} anon_page_t;

typedef uint64_t off_t;
typedef struct vm_page_s {
	int ref;
	vmobject_t * object;
	off_t offset;

	int flags;
} vm_page_t;

typedef void (*segment_faulter)(struct segment_s * seg, void * p, int write, int user, int present);
typedef struct segment_s {
	void * base;
	size_t size;
	int perms;

	/* Writes go to dirty object */
	vmobject_t * dirty;

	/* Reads come from clean object, if they're not in dirty object */
	off_t read_offset;
	vmobject_t * clean;
} segment_t;
#endif

typedef struct segment_anonymous_s {
	segment_t segment;
}segment_anonymous_t;

map_t * kas;
static slab_type_t segments[1] = {SLAB_TYPE(sizeof(segment_t), 0, 0)};
static slab_type_t objects[1] = {SLAB_TYPE(sizeof(vmobject_t), 0, 0)};
void vm_init()
{
	INIT_ONCE();

	tree_init();
	kas = tree_new(0, TREE_TREAP);
}

static void vm_invalid_pointer(void * p, int write, int user, int present)
{
	kernel_panic("Invalid pointer: %p\n", p);
}

void vm_page_fault(void * p, int write, int user, int present)
{
	map_t * as = 0;
	thread_lock(kas);
	segment_t * seg = map_getpp_le(kas, p);
	thread_unlock(kas);
	if (0 == seg) {
		as = arch_get_thread()->as;
		thread_lock(as);
		seg = map_getpp_le(as, p);
		thread_unlock(as);
	}

	if (seg) {

		long offset = (char*)p - (char*)seg->base;

		if (offset < seg->size) {
			if (!present) {
				page_t page = seg->dirty->ops->get_page(seg->dirty, offset);
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

/*
 * Anonymous private memory object
 */
static page_t vm_anon_get_page(vmobject_t * anon, int offset)
{
	page_t page = vector_get(anon->anon.pages, offset >> ARCH_PAGE_SIZE_LOG2);

	if (!page) {
		page = page_alloc();
	}

	return page;
}

static vmobject_ops_t anon_ops = {
	get_page: vm_anon_get_page
};

static void vm_object_anon_copy_pages(vector_t * v, void * arg, int i, void * p)
{
	vm_page_t * vmp = p;
	vector_t * to = arg;

	/* Increment the reference count on the page */
	arch_atomic_postinc(&vmp->ref);
	vector_putp(to, i, vmp);
}

static vmobject_t * vm_object_anon_copy(vmobject_t * from)
{
	check_int_is(from->type, OBJECT_ANON, "Clone object is not anonymous");
	vmobject_t * anon = slab_alloc(objects);
	anon->ops = &anon_ops;
	anon->type = OBJECT_ANON;
	anon->anon.pages = vector_new();
	
	return anon;
}

static vmobject_t * vm_object_anon()
{
	vmobject_t * anon = slab_alloc(objects);
	anon->ops = &anon_ops;
	anon->type = OBJECT_ANON;
	anon->anon.pages = vector_new();
	return anon;
}

/*
 * Direct mapped (eg - device) memory
 */
static page_t vm_direct_get_page(vmobject_t * direct, int offset)
{
	if (offset<direct->direct.size) {
		return direct->direct.base + (offset >> ARCH_PAGE_SIZE_LOG2);
	}
	/* FIXME: Throw an exception? */
	return 0;
}

static vmobject_ops_t direct_ops = {
	get_page: vm_direct_get_page
};

static vmobject_t * vm_object_direct( page_t base, int size)
{
	vmobject_t * direct = slab_alloc(objects);
	direct->ops = &direct_ops;
	direct->type = OBJECT_ANON;
	direct->direct.base = base;
	direct->direct.size = size;
	return direct;
}

segment_t * vm_segment_base( void * p, size_t size, int perms, vmobject_t * clean, int offset)
{
	vm_init();
	segment_t * seg = slab_alloc(segments);

	seg->base = p;
	seg->size = size;
	seg->perms = perms;
	seg->clean = clean;
	seg->read_offset = offset;
	seg->dirty = clean;

	return seg;
}

segment_t * vm_segment_anonymous(void * p, size_t size, int perms)
{
	segment_t * seg = vm_segment_base(p, size, perms | SEGMENT_P, 0, 0);
	seg->dirty = vm_object_anon();

	return seg;
}

segment_t * vm_segment_direct(void * p, size_t size, int perms, page_t base)
{
	vmobject_t * object = vm_object_direct(base, size);
	segment_t * seg = vm_segment_base(p, size, perms, object, 0);

	return seg;
}

segment_t * vm_segment_copy(segment_t * from, int private)
{
	segment_t * seg = vm_segment_base(from->base, from->size, from->perms, from->clean, from->read_offset);

	if (private) {
		seg->perms |= SEGMENT_P;

		if (from->clean != from->dirty) {
			/* Initialize dirty from old segment dirty map */
			seg->dirty = vm_object_anon_copy(from->dirty);
		} else {
			/* Empty dirty object */
			seg->dirty = vm_object_anon();
		}
	}

	return seg;
}
