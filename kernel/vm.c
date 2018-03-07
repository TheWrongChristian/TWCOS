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

enum object_type_e { OBJECT_DIRECT, OBJECT_ANON, OBJECT_VNODE };

typedef struct vmobject_ops_s {
	page_t (*get_page)(struct vmobject_s * object, off_t offset);
	void (*put_page)(struct vmobject_s * object, off_t offset, page_t page);
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
			map_t * pages;
		} anon;
		struct {
			vnode_t * vnode;
		} vnode;
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

typedef struct address_info_t {
	map_t * as;
	segment_t * seg;
	off_t offset;
} address_info_t;

#define VMPAGE_PINNED 0x1
#define VMPAGE_ACCESSED 0x2
#define VMPAGE_DIRTY 0x4

#endif

typedef struct segment_anonymous_s {
	segment_t segment;
}segment_anonymous_t;

static map_t * vmpages;
map_t * kas;
static slab_type_t segments[1] = {SLAB_TYPE(sizeof(segment_t), 0, 0)};
static slab_type_t objects[1] = {SLAB_TYPE(sizeof(vmobject_t), 0, 0)};
void vm_init()
{
	INIT_ONCE();

	tree_init();
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
	thread_lock(as);
	segment_t * seg = map_getpp_cond(as, p, MAP_LE);
	thread_unlock(as);

	return seg;
}

static int vm_resolve_address(void * p, address_info_t * info)
{
	map_t * as = kas;
	segment_t * seg = vm_get_segment(as, p);
	if (0 == seg) {
		as = arch_get_thread()->as;
		if (as) {
			seg = vm_get_segment(kas, p);
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
				page_t page = seg->dirty->ops->get_page(seg->dirty, offset);
				if (0 == page) {
					/* Handle case of page not in dirty pages */
					page = seg->clean->ops->get_page(seg->clean, offset + seg->read_offset);
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

/*
 * Anonymous private memory object
 */
static page_t vm_anon_get_page(vmobject_t * anon, off_t offset)
{
	page_t page = map_get(anon->anon.pages, offset >> ARCH_PAGE_SIZE_LOG2);

	if (!page) {
		page = page_alloc();
		map_put(anon->anon.pages, offset >> ARCH_PAGE_SIZE_LOG2, page);
	}

	return page;
}

static void vm_object_anon_copy_pages(void * arg, int i, void * p)
{
	vm_page_t * vmp = p;
	map_t * to = arg;

	/* Increment the reference count on the page */
	arch_atomic_postinc(&vmp->ref);
	map_putip(to, i, vmp);
}

static void vm_object_anon_copy_walk(void * p, map_key key, map_data data)
{
	vmobject_t * anon = (vmobject_t *)p;

	map_put(anon->anon.pages, key, data);
}

static vmobject_t * vm_object_anon()
{
	static vmobject_ops_t anon_ops = {
		get_page: vm_anon_get_page,
	};

	vmobject_t * anon = slab_alloc(objects);
	anon->ops = &anon_ops;
	anon->type = OBJECT_ANON;
	anon->anon.pages = vector_new();

	return anon;
}

static vmobject_t * vm_object_anon_copy(vmobject_t * from)
{
	check_int_is(from->type, OBJECT_ANON, "Clone object is not anonymous");
	vmobject_t * anon = vm_object_anon();
	map_walk(from->anon.pages, vm_object_anon_copy_walk, anon);
	
	return anon;
}

/*
 * Direct mapped (eg - device) memory
 */
static page_t vm_direct_get_page(vmobject_t * direct, off_t offset)
{
	if (offset<direct->direct.size) {
		return direct->direct.base + (offset >> ARCH_PAGE_SIZE_LOG2);
	}
	/* FIXME: Throw an exception? */
	return 0;
}

static vmobject_t * vm_object_direct( page_t base, int size)
{
	static vmobject_ops_t direct_ops = {
		get_page: vm_direct_get_page
	};

	vmobject_t * direct = slab_alloc(objects);
	direct->ops = &direct_ops;
	direct->type = OBJECT_ANON;
	direct->direct.base = base;
	direct->direct.size = size;
	return direct;
}

static page_t vm_vnode_get_page(vmobject_t * o, off_t offset)
{
	return vnode_get_page(o->vnode.vnode, offset);
}

static vmobject_t * vm_object_vnode(vnode_t * vnode)
{
	static vmobject_ops_t vnode_ops = {
		get_page: vm_vnode_get_page
	};

	vmobject_t * ovnode = slab_alloc(objects);
        ovnode->ops = &vnode_ops;
        ovnode->type = OBJECT_VNODE;
        ovnode->vnode.vnode = vnode;
        return ovnode;
}

segment_t * vm_segment_base( void * p, size_t size, int perms, vmobject_t * clean, off_t offset)
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

segment_t * vm_segment_vnode(void * p, size_t size, int perms, vnode_t * vnode, off_t offset)
{
	vmobject_t * vobject = vm_object_vnode(vnode);
	segment_t * seg = vm_segment_base(p, size, perms | SEGMENT_P, vobject, offset);
	if (perms & SEGMENT_P) {
		seg->dirty = vm_object_anon();
	}

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
		
		page_t page = seg->dirty->ops->get_page(seg->dirty, offset >> ARCH_PAGE_SIZE_LOG2);
		if (page) {
			seg->dirty->ops->put_page(seg->dirty, offset, 0);
			vm_vmpage_unmap(page, as, p);
			if (vmap_ismapped(as, p)) {
				vmap_unmap(as, p);
			}
		} else {
			page = page_alloc();
		}
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

	arch_spin_lock(lock);
	uintptr_t p = (uintptr_t)kas_next;
	p += (align-1);
	p &= ~(align-1);
	kas_next = (void*)p + size;
	arch_spin_unlock(lock);

	return (void*)p;
}

void * vm_kas_get( size_t size )
{
	return vm_kas_get_aligned(size, sizeof(intptr_t));
}

static int vmpages_lock;

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
	SPIN_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);
		int count = 1;

		if (vmpage) {
			/* Check if we already have this mapping */
			for(int i=0; i<vmpage->count; i++) {
				if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
					spin_unlock(&vmpages_lock);
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
	SPIN_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			for(int i=0; i<vmpage->count; i++) {
				if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
					vmpage->maps[i].as = vmpage->maps[vmpage->count-1].as;
					vmpage->maps[i].p = vmpage->maps[vmpage->count-1].p;
					i = vmpage->count--;
					spin_unlock(&vmpages_lock);
					return;
				}
			}
		}
	}
}

void vm_vmpage_setflags(page_t page, int flags)
{
	SPIN_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			vmpage->flags |= flags;
		}
	}
}

void vm_vmpage_resetflags(page_t page, int flags)
{
	SPIN_AUTOLOCK(&vmpages_lock) {
		vmpage_t * vmpage = map_getip(vmpages, page);

		if (vmpage)  {
			vmpage->flags &= ~flags;
		}
	}
}

void vm_vmpage_trapwrites(page_t page)
{
	SPIN_AUTOLOCK(&vmpages_lock) {
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
	SPIN_AUTOLOCK(&vmpages_lock) {
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
	SPIN_AUTOLOCK(&vmpages_lock) {
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
