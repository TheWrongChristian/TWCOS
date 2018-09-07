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
#include <stdint.h>

#define SEGMENT_R 0x1
#define SEGMENT_W 0x2
#define SEGMENT_X 0x4
#define SEGMENT_U 0x8
#define SEGMENT_P 0x10

enum object_type_e { OBJECT_DIRECT, OBJECT_ANON, OBJECT_VNODE };

struct vmobject_ops_t {
	page_t (*get_page)(vmobject_t * object, off_t offset);
	page_t (*put_page)(vmobject_t * object, off_t offset, page_t page);
	vmobject_t * (*clone)(vmobject_t * object);
};

struct vmobject_t {
	vmobject_ops_t * ops;
#if 0
	/* Per object type data */
	int type;
	union {
		struct {
			page_t base;
			size_t size;
		} direct;
		struct {
			map_t * pages;
			vmobject_t * clean;
		} anon;
		struct {
			vnode_t * vnode;
		} vnode;
	};
#endif
};

typedef uint64_t off_t;
struct vm_page_t {
	int ref;
	vmobject_t * object;
	off_t offset;

	int flags;
};

struct segment_t {
	void * base;
	size_t size;
	int perms;

	/* Writes go to dirty object */
	off_t dirty_offset;
	vmobject_t * dirty;

	/* Reads come from clean object, if they're not in dirty object */
	off_t read_offset;
	vmobject_t * clean;
};

struct address_info_t {
	map_t * as;
	segment_t * seg;
	off_t offset;
};

#define VMPAGE_PINNED 0x1
#define VMPAGE_ACCESSED 0x2
#define VMPAGE_DIRTY 0x4

#endif

typedef struct segment_anonymous_s segment_anonymous_t;
struct segment_anonymous_s {
	segment_t segment;
};

static map_t * vmpages;
map_t * kas;

void vm_init()
{
	INIT_ONCE();

	kas = tree_new(0, TREE_TREAP);
	vmpages = vector_new();
	thread_gc_root(kas);
	thread_gc_root(vmpages);
}

static void vm_invalid_pointer(void * p, int write, int user, int present)
{
	kernel_panic("Invalid pointer: %p\n", p);
}

static segment_t * vm_get_segment(map_t * as, void * p)
{
	/* Check for kernel address space */
	segment_t * seg = map_getpp_cond(as, p, MAP_LE);

	return seg;
}

static mutex_t kaslock = {0};

static int vm_resolve_address(void * p, address_info_t * info)
{
	segment_t * seg = 0;
	map_t * as = kas;

	MUTEX_AUTOLOCK(&kaslock) {
		seg = vm_get_segment(as, p);
		if (0 == seg) {
			as = (arch_get_thread()->process) ? arch_get_thread()->process->as : 0;
			if (as) {
				seg = vm_get_segment(as, p);
			}
		}
	}

	if (seg) {
		info->as = as;
		info->seg = seg;
		info->offset = (char*)p - (char*)seg->base;

		return 1;
	}

	return 0;
}

void vm_page_fault(void * p, int write, int user, int present)
{
	address_info_t info[1];

	if (vm_resolve_address(p, info)) {
		segment_t * seg = info->seg;
		off_t offset = info->offset;
		map_t * as = info->as;

		if (offset < seg->size) {
			/* Adjust p to page boundary */
			p = ARCH_PAGE_ALIGN(p);
			if (!present) {
				/* FIXME: This all needs review */
				page_t page = vmobject_get_page(seg->dirty, offset);
				if (0 == page) {
					/* Handle case of page not in dirty pages */
					page = vmobject_get_page(seg->clean, offset + seg->read_offset);
					vmobject_put_page(seg->dirty, offset, page);
					vmap_map(as, p, page, 0, SEGMENT_U & seg->perms);
				} else {
					vmap_map(as, p, page, write && SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
				}
				vm_vmpage_map(page, as, p);
				vm_vmpage_setflags(page, VMPAGE_ACCESSED | (write) ? VMPAGE_DIRTY : 0);
			} else if (write && SEGMENT_W & seg->perms) {
				page_t page = vmap_get_page(as, p);
				vmap_map(as, p, page, write && SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
				vm_vmpage_map(page, as, p);
				vm_vmpage_setflags(page, VMPAGE_ACCESSED | VMPAGE_DIRTY);
			} else {
				vm_invalid_pointer(p, write, user, present);
			}

			return;
		}
	}

	/* Invalid pointer */
	vm_invalid_pointer(p, write, user, present);
}

page_t vmobject_get_page(vmobject_t * object, off_t offset)
{
	return object->ops->get_page(object, offset);
}

page_t vmobject_put_page(vmobject_t * object, off_t offset, page_t page)
{
	return object->ops->put_page(object, offset, page);
}

/*
 * Zero filled memory
 */
static page_t vm_zero_get_page(vmobject_t * object, off_t offset)
{
	return page_calloc();
}

static vmobject_t * vm_zero_clone(vmobject_t * object)
{
	return object;
}

static vmobject_t * vm_object_zero()
{
	static vmobject_ops_t anon_ops = {
		get_page: vm_zero_get_page,
		clone: vm_zero_clone
	};

	static vmobject_t zero = {
		ops: &anon_ops
	};

	return &zero;
}

/*
 * Anonymous private memory object
 */
typedef struct vmobject_anon_t vmobject_anon_t;
struct vmobject_anon_t {
	vmobject_t vmobject;
	map_t * pages;
};

static page_t vm_anon_get_page(vmobject_t * object, off_t offset)
{
	vmobject_anon_t * anon = container_of(object, vmobject_anon_t, vmobject);
	page_t page = map_get(anon->pages, offset >> ARCH_PAGE_SIZE_LOG2);

#if 0
	if (!page) {
		page = page_calloc();
		map_put(anon->pages, offset >> ARCH_PAGE_SIZE_LOG2, page);
	}
#endif

	return page;
}

static page_t vm_anon_put_page(vmobject_t * object, off_t offset, page_t page)
{
	vmobject_anon_t * anon = container_of(object, vmobject_anon_t, vmobject);
	return map_put(anon->pages, offset >> ARCH_PAGE_SIZE_LOG2, page);
}

static void vm_object_anon_copy_walk(void * p, map_key key, map_data data)
{
	vmobject_anon_t * anon = (vmobject_anon_t *)p;

	map_put(anon->pages, key, data);
}

static vmobject_t * vm_anon_clone(vmobject_t * object);
static vmobject_t * vm_object_anon()
{
	static vmobject_ops_t anon_ops = {
		get_page: vm_anon_get_page,
		put_page: vm_anon_put_page,
		clone: vm_anon_clone
	};

	vmobject_anon_t * anon = calloc(1, sizeof(*anon));
	anon->vmobject.ops = &anon_ops;
	anon->pages = vector_new();

	return &anon->vmobject;
}

static vmobject_t * vm_anon_clone(vmobject_t * object)
{
	vmobject_anon_t * from = container_of(object, vmobject_anon_t, vmobject);
	vmobject_anon_t * anon = container_of(vm_object_anon(), vmobject_anon_t, vmobject);
	map_walk(from->pages, vm_object_anon_copy_walk, anon);

	return &anon->vmobject;
}

/*
 * Direct mapped (eg - device) memory
 */
typedef struct vmobject_direct_t vmobject_direct_t;
struct vmobject_direct_t {
	vmobject_t vmobject;
	page_t base;
	size_t size;
};

static page_t vm_direct_get_page(vmobject_t * object, off_t offset)
{
	vmobject_direct_t * direct = container_of(object, vmobject_direct_t, vmobject);
	if (offset<direct->size) {
		return direct->base + (offset >> ARCH_PAGE_SIZE_LOG2);
	}
	/* FIXME: Throw an exception? */
	return 0;
}

static vmobject_t * vm_object_direct( page_t base, int size);
static vmobject_t * vm_direct_clone(vmobject_t * object)
{
	vmobject_direct_t * from = container_of(object, vmobject_direct_t, vmobject);
	vmobject_direct_t * direct = container_of(vm_object_direct(from->base, from->size), vmobject_direct_t, vmobject);
	return &direct->vmobject;
}

static vmobject_t * vm_object_direct( page_t base, int size)
{
	static vmobject_ops_t direct_ops = {
		get_page: vm_direct_get_page,
		clone: vm_direct_clone
	};

	vmobject_direct_t * direct = calloc(1, sizeof(*direct));
	direct->vmobject.ops = &direct_ops;
	direct->base = base;
	direct->size = size;
	return &direct->vmobject;
}

/*
 * vnode mapped memory
 */
typedef struct vmobject_vnode_t vmobject_vnode_t;
struct vmobject_vnode_t {
	vmobject_t vmobject;
	vnode_t * vnode;
};

static page_t vm_vnode_get_page(vmobject_t * object, off_t offset)
{
	vmobject_vnode_t * vno = container_of(object, vmobject_vnode_t, vmobject);

	return vnode_get_page(vno->vnode, offset);
}

static vmobject_t * vm_object_vnode(vnode_t * vnode);
static vmobject_t * vm_object_clone(vmobject_t * object)
{
	vmobject_vnode_t * vno = container_of(object, vmobject_vnode_t, vmobject);

	return vm_object_vnode(vno->vnode);
}

static vmobject_t * vm_object_vnode(vnode_t * vnode)
{
	static vmobject_ops_t vnode_ops = {
		get_page: vm_vnode_get_page,
		clone: vm_object_clone
	};

	vmobject_vnode_t * ovnode = calloc(1, sizeof(*ovnode));
        ovnode->vmobject.ops = &vnode_ops;
        ovnode->vnode = vnode;
        return &ovnode->vmobject;
}

segment_t * vm_segment_base( void * p, size_t size, int perms, vmobject_t * clean, off_t offset)
{
	vm_init();
	segment_t * seg = calloc(1, sizeof(*seg));

	seg->base = p;
	seg->size = size;
	seg->perms = perms;
	seg->clean = clean;
	seg->read_offset = offset;
	seg->dirty = clean;

	return seg;
}

segment_t * vm_segment_vnode(void * p, size_t size, int perms, vnode_t * vnode, off_t offset)
{
	vmobject_t * vobject = vm_object_vnode(vnode);
	segment_t * seg = vm_segment_base(p, size, perms | SEGMENT_P, vobject, offset);
	if (perms & SEGMENT_P) {
		seg->dirty = vm_object_anon(vobject);
	}

	return seg;
}

segment_t * vm_segment_anonymous(void * p, size_t size, int perms)
{
	segment_t * seg = vm_segment_base(p, size, perms | SEGMENT_P, vm_object_zero(), 0);
	seg->dirty = vm_object_anon(0);

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
			seg->dirty = vm_object_clone(from->dirty);
		} else {
			/* Empty dirty object */
			seg->dirty = vm_object_anon();
		}
	} else {
		seg->dirty = from->dirty;
	}

	return seg;
}

page_t vm_page_steal(void * p)
{
	address_info_t info[1];

	if (vm_resolve_address(p, info)) {
		segment_t * seg = info->seg;
		off_t offset = info->offset;
		map_t * as = info->as;

		if (!(seg->perms & SEGMENT_P)) {
			/* Can't steal from non-private segments */
			return 0;
		}
		
		page_t page = vmobject_get_page(seg->dirty, offset);
		if (page) {
			vmobject_put_page(seg->dirty, offset, 0);
			vm_vmpage_unmap(page, as, p);
			if (vmap_ismapped(as, p)) {
				vmap_unmap(as, p);
			}
		} else {
			page = page_calloc();
		}

		return page;
	}

	return 0;
}

static void * kas_next;

void vm_kas_start(void * p)
{
	kas_next = p;
}

void * vm_kas_get_aligned( size_t size, size_t align )
{
	static int lock[1];

	spin_lock(lock);
	uintptr_t p = (uintptr_t)kas_next;
	p += (align-1);
	p &= ~(align-1);
	kas_next = (void*)p + size;
	spin_unlock(lock);

	return (void*)p;
}

void * vm_kas_get( size_t size )
{
	return vm_kas_get_aligned(size, sizeof(intptr_t));
}

static mutex_t vmpages_lock;

typedef struct vmpage_s {
	int count;
	int flags;
	int age;
	struct {
		asid as;
		void * p;
	} maps[];
} vmpage_t;

void vm_vmpage_map( page_t page, asid as, void * p )
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);
		int count = 1;

		if (vmpage) {
			/* Check if we already have this mapping */
			for(int i=0; i<vmpage->count; i++) {
				if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
					mutex_unlock(&vmpages_lock);
					return;
				}
			}

			count = vmpage->count + 1;
		}
		vmpage = realloc(vmpage, sizeof(*vmpage) + count*sizeof(vmpage->maps[0]));
		vmpage->count = count;
		vmpage->maps[count-1].as = as;
		vmpage->maps[count-1].p = p;

		/* vmpage might have changed in realloc, update it */
		map_putip(vmpages, page, vmpage);
	}
}

void vm_vmpage_unmap( page_t page, asid as, void * p )
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			for(int i=0; i<vmpage->count; i++) {
				if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
					vmpage->maps[i].as = vmpage->maps[vmpage->count-1].as;
					vmpage->maps[i].p = vmpage->maps[vmpage->count-1].p;
					i = vmpage->count--;
					mutex_unlock(&vmpages_lock);
					return;
				}
			}
		}
	}
}

void vm_vmpage_setflags(page_t page, int flags)
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			vmpage->flags |= flags;
		}
	}
}

void vm_vmpage_resetflags(page_t page, int flags)
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			vmpage->flags &= ~flags;
		}
	}
}

void vm_vmpage_trapwrites(page_t page)
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);
		if (vmpage)  {
			for(int i=0; i<vmpage->count; i++) {
				asid as = vmpage->maps[i].as;
				void * p = vmpage->maps[i].p;

				/* Mark each mapping as read only */
				if (vmap_ismapped(as, p)) {
					//int user = vmap_isuser(as, p);
					//vmap_map(as, p, page, 0, user);
					vmap_unmap(as, p);
				}
			}
		}
	}
}

void vm_vmpage_trapaccess(page_t page)
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);
		if (vmpage)  {
			for(int i=0; i<vmpage->count; i++) {
				asid as = vmpage->maps[i].as;
				void * p = vmpage->maps[i].p;

				/* Remove each mapping */
				if (vmap_ismapped(as, p)) {
					vmap_unmap(as, p);
				}
			}
		}
	}
}

void vm_vmpage_age(page_t page)
{
	MUTEX_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);
		if (vmpage)  {
			if (vmpage->age) {
				/* Already referenced before, add age */
				vmpage->age >>= 1;
				if (vmpage->flags & VMPAGE_ACCESSED) {
					vmpage->age |= 0x100;
				}
			} else {
				/* First use */
				vmpage->age = (vmpage->flags & VMPAGE_ACCESSED) ? 4 : 0;
			}
		}
	}
}
