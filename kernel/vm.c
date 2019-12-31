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
	vmpage_t * (*get_page)(vmobject_t * object, off_t offset);
	vmpage_t * (*put_page)(vmobject_t * object, off_t offset, vmpage_t * page);
	void (*release)(vmobject_t * object);
	vmobject_t * (*clone)(vmobject_t * object);
};

struct vmobject_t {
	vmobject_ops_t * ops;
};

typedef uint64_t off_t;

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
#define VMPAGE_MANAGED 0x8
#define VMPAGE_MAPS 3
typedef struct vmpage_t {
	page_t page;
	int flags;
	int age;
	int copies;
	struct {
		asid as;
		void * p;
	} maps[VMPAGE_MAPS];
};

#endif

typedef struct segment_anonymous_s segment_anonymous_t;
struct segment_anonymous_s {
	segment_t segment;
};

void vmpage_mark(void * p)
{
	vmpage_t * vmpage = p;
#if 0
	if (vmpage->flags & VMPAGE_MANAGED) {
		page_gc_mark(vmpage->page);
	}
#endif
}

static void vmpage_finalize(void * p);
static slab_type_t slabvmpages[] = { SLAB_TYPE(sizeof(vmpage_t), vmpage_mark, vmpage_finalize) };

static GCROOT map_t * vmpages;
GCROOT map_t * kas;
#define RMAP 0
void vm_init()
{
	INIT_ONCE();

	kas = tree_new(0, TREE_TREAP);
#if RMAP
	vmpages = vector_new();
#endif
}

static void vm_invalid_pointer(void * p, int write, int user, int present)
{
	dump_alloc_audit(p);
	kernel_panic("Invalid pointer: %p\n", p);
}

segment_t * vm_get_segment(map_t * as, void * p)
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

void vm_kas_add(segment_t * seg)
{
	MUTEX_AUTOLOCK(&kaslock) {
		map_putpp(kas, seg->base, seg);
	}
}

void vm_kas_remove(segment_t * seg)
{
	MUTEX_AUTOLOCK(&kaslock) {
		assert(seg == map_removepp(kas, seg->base));
		vm_segment_unmap(kas, seg);
	}
}

void vm_page_fault(void * p, int write, int user, int present)
{
	address_info_t info[1];

	if (vm_resolve_address(p, info)) {
		segment_t * seg = info->seg;
		off_t offset = info->offset;
		map_t * as = info->as;

		if (offset < seg->size) {
			/* FIXME: This all needs review */
			if (seg == heap) {
				write=1;
			}

			/* Check for invalid writes first */
			if (write && !(SEGMENT_W & seg->perms)) {
				vm_invalid_pointer(p, write, user, present);
				return;
			}

			/* Adjust p to page boundary */
			p = ARCH_PAGE_ALIGN(p);

			/* If not present, load a page */
			vmpage_t * vmpage = vmobject_get_page(seg->dirty, offset);
			if (0 == vmpage) {
				/* Handle case of page not in dirty pages */
				vmpage = vmobject_get_page(seg->clean, offset + seg->read_offset);
				if (write && seg->clean != seg->dirty) {
					vmpage_put_copy(vmpage);
					vmobject_put_page(seg->dirty, offset, vmpage);
				}
			}

			/* By here, page is the page we want, and is present */
			if (write && SEGMENT_W & seg->perms) {
				vmpage_t * newpage = vmpage_get_copy(vmpage);
				if (newpage != vmpage) {
					vmpage_unmap(vmpage, as, p);
					vmobject_put_page(seg->dirty, offset, newpage);
					vmpage = newpage;
				}
			}

			/* By here, page is present, and a copy if COW */
			vmpage_map(vmpage, as, p, write && SEGMENT_W & seg->perms, SEGMENT_U & seg->perms);
			if (write) {
				vmpage_setflags(vmpage, VMPAGE_ACCESSED | VMPAGE_DIRTY);
			} else {
				vmpage_setflags(vmpage, VMPAGE_ACCESSED);
			}

			/*
			 * Don't persist the vmpage_t structs for heap pages.
			 * We must ensure we unreference the page backing the
			 * heap vmpage_t, otherwise the page will be released
			 * once the vmpage_t is GCed.
			 */
			if (seg == heap) {
				memset(vmpage, 0, sizeof(*vmpage));
			}

			return;
		}
	}

	/* Invalid pointer */
	vm_invalid_pointer(p, write, user, present);
}

vmpage_t * vmobject_get_page(vmobject_t * object, off_t offset)
{
	if (object->ops->get_page) {
		vmpage_t * vmpage = object->ops->get_page(object, offset);
		if (vmpage) {
			assert(vmpage->page);
		}
		return vmpage;
	}

	KTHROWF(RuntimeException, "Unimplemented vmobject_get_page: %p", object);

	return 0;
}

vmpage_t * vmobject_put_page(vmobject_t * object, off_t offset, vmpage_t * vmpage)
{
	if (vmpage) {
		assert(vmpage->page);
	}
	if (object->ops->put_page) {
		return object->ops->put_page(object, offset, vmpage);
	}

	KTHROWF(RuntimeException, "Unimplemented vmobject_put_page: %p", object);

	return 0;
}

vmobject_t * vmobject_clone(vmobject_t * object)
{
	if (object->ops->clone) {
		return object->ops->clone(object);
	}

	KTHROWF(RuntimeException, "Unimplemented vmobject_clone: %p", object);

	return 0;
}

void vmobject_release(vmobject_t * object)
{
	if (object->ops->release) {
		object->ops->release(object);
	}
	/* Do nothing if not defined */
}

static void vm_as_release_walk(void * p, void * key, void * data)
{
	segment_t * seg = (segment_t *)data;

	if (seg->dirty && seg->dirty != seg->clean) {
		/* Clean dirty segment */
		vmobject_release(seg->dirty);
	}
	vmobject_release(seg->clean);
}

void vm_as_release(map_t * as)
{
	vmap_release_asid(as);
}

/*
 * Zero filled memory
 */
static vmpage_t * vm_zero_get_page(vmobject_t * object, off_t offset)
{
	return vmpage_calloc();
}

static vmobject_t * vm_zero_clone(vmobject_t * object)
{
	return object;
}

static vmobject_t * vm_object_zero()
{
	static vmobject_ops_t zero_ops = {
		get_page: vm_zero_get_page,
		clone: vm_zero_clone
	};

	static vmobject_t zero = {
		ops: &zero_ops
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

static vmpage_t * vm_anon_get_page(vmobject_t * object, off_t offset)
{
	vmobject_anon_t * anon = container_of(object, vmobject_anon_t, vmobject);
	return map_getip(anon->pages, offset >> ARCH_PAGE_SIZE_LOG2);
}

static vmpage_t * vm_anon_put_page(vmobject_t * object, off_t offset, vmpage_t * vmpage)
{
	vmobject_anon_t * anon = container_of(object, vmobject_anon_t, vmobject);
	return map_putip(anon->pages, offset >> ARCH_PAGE_SIZE_LOG2, vmpage);
}

static void vm_anon_release_walk(void * p, map_key key, void * data)
{
	vmpage_release(data);
}

static void vm_anon_release(vmobject_t * object)
{
	vmobject_anon_t * anon = container_of(object, vmobject_anon_t, vmobject);
	map_walkip(anon->pages, vm_anon_release_walk, anon);
}

static void vm_object_anon_copy_walk(void * p, map_key key, void * data)
{
	vmobject_anon_t * anon = (vmobject_anon_t *)p;
	vmpage_t * vmpage = (vmpage_t *)data;

	/* Mark COW on now shared page */
	vmpage_put_copy(vmpage);
	map_putip(anon->pages, key, vmpage);
}

static vmobject_t * vm_object_anon();
static vmobject_t * vm_anon_clone(vmobject_t * object)
{
	vmobject_anon_t * from = container_of(object, vmobject_anon_t, vmobject);
	vmobject_anon_t * anon = container_of(vm_object_anon(), vmobject_anon_t, vmobject);
	map_walkip(from->pages, vm_object_anon_copy_walk, anon);

	return &anon->vmobject;
}

static vmobject_t * vm_object_anon()
{
	static vmobject_ops_t anon_ops = {
		get_page: vm_anon_get_page,
		put_page: vm_anon_put_page,
		release: vm_anon_release,
		clone: vm_anon_clone
	};

	vmobject_anon_t * anon = calloc(1, sizeof(*anon));
	anon->vmobject.ops = &anon_ops;
	anon->pages = vector_new();

	return &anon->vmobject;
}

/*
 * Special singleton kernel heap object
 */
typedef struct vmobject_heap_t vmobject_heap_t;
struct vmobject_heap_t {
	vmobject_t vmobject;
	int pcount;
	page_t * pages;
};

static vmpage_t * vm_heap_get_page(vmobject_t * object, off_t offset)
{
	vmobject_heap_t * heap = container_of(object, vmobject_heap_t, vmobject);
	int pageno = offset >> ARCH_PAGE_SIZE_LOG2;

	if (pageno<heap->pcount) {
		page_t page = heap->pages[pageno];
		if (0 == page) {
			page = page_alloc();
			heap->pages[pageno] = page;
		}
		return vmpage_alloc(0, page);
	}

	kernel_panic("Get page beyond end of heap");
	return 0;
}

static vmobject_t * vm_heap_put_page(vmobject_t * object, off_t offset, vmpage_t * vmpage)
{
	vmobject_heap_t * heap = container_of(object, vmobject_heap_t, vmobject);
	int pageno = offset >> ARCH_PAGE_SIZE_LOG2;

	if (pageno<heap->pcount) {
		heap->pages[pageno] = vmpage->page;
		vmpage->page = 0;
		return 0;
	}

	kernel_panic("Put page beyond end of heap");
	return 0;
}

vmobject_t * vm_object_heap(int pcount)
{
	static vmobject_ops_t heap_ops = {
		get_page: vm_heap_get_page,
		put_page: vm_heap_put_page,
	};
	static vmobject_heap_t heap = {vmobject: {&heap_ops}};
	if (!heap.pages) {
		heap.pcount = pcount;
		heap.pages = bootstrap_alloc(sizeof(*heap.pages)*pcount);
		for(int i=0; i<pcount; i++) {
			heap.pages[i] = 0;
		}
	}
	return &heap.vmobject;
}

/*
 * Direct mapped (eg - device) memory
 */
typedef struct vmobject_direct_t vmobject_direct_t;
struct vmobject_direct_t {
	vmobject_t vmobject;
	map_t * pages;
	page_t base;
	size_t size;
};

static vmpage_t * vm_direct_get_page(vmobject_t * object, off_t offset)
{
	vmobject_direct_t * direct = container_of(object, vmobject_direct_t, vmobject);
	if (offset<direct->size) {
		int pageno = offset >> ARCH_PAGE_SIZE_LOG2;
		vmpage_t * vmpage = map_getip(direct->pages, pageno);
		if (0 == vmpage) {
			vmpage = vmpage_alloc(0, direct->base + pageno);
			map_putip(direct->pages, pageno, vmpage);
		}
		return vmpage;
	}
	/* FIXME: Throw an exception? */
	return 0;
}

static vmobject_t * vm_object_direct( page_t base, int size);
static vmobject_t * vm_direct_clone(vmobject_t * object)
{
	vmobject_direct_t * from = container_of(object, vmobject_direct_t, vmobject);
	vmobject_direct_t * direct = container_of(vm_object_direct(from->base, from->size), vmobject_direct_t, vmobject);
	direct->pages = from->pages;
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
	direct->pages = vector_new();
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

static vmpage_t * vm_vnode_get_page(vmobject_t * object, off_t offset)
{
	vmobject_vnode_t * vno = container_of(object, vmobject_vnode_t, vmobject);

	return vnode_get_page(vno->vnode, offset);
}

static vmobject_t * vm_object_vnode(vnode_t * vnode);
static vmobject_t * vm_vnode_clone(vmobject_t * object)
{
	vmobject_vnode_t * vno = container_of(object, vmobject_vnode_t, vmobject);

	return vm_object_vnode(vno->vnode);
}

static vmobject_t * vm_object_vnode(vnode_t * vnode)
{
	static vmobject_ops_t vnode_ops = {
		get_page: vm_vnode_get_page,
		clone: vm_vnode_clone
	};

	vmobject_vnode_t * ovnode = calloc(1, sizeof(*ovnode));
        ovnode->vmobject.ops = &vnode_ops;
        ovnode->vnode = vnode;
        return &ovnode->vmobject;
}

void vm_segment_unmap(asid as, segment_t * seg)
{
	char * p = seg->base;
	for(size_t offset=0; offset<seg->size; offset+=ARCH_PAGE_SIZE) {
		vmpage_t * vmpage = vmobject_get_page(seg->dirty, offset);
		if (0 == vmpage) {
			/* Handle case of page not in dirty pages */
			vmpage = vmobject_get_page(seg->clean, offset);
		}
		if (vmpage) {
			/* Page might be mapped, unmap it */
			vmpage_unmap(vmpage, as, p+offset);
		}
	}
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
	segment_t * seg = vm_segment_base(p, size, perms, vobject, offset);
	if (perms & SEGMENT_P) {
		seg->dirty = vm_object_anon(vobject);
	}

	return seg;
}

segment_t * vm_segment_anonymous(void * p, size_t size, int perms)
{
	segment_t * seg = vm_segment_base(p, size, perms | SEGMENT_P, vm_object_zero(), 0);
	seg->dirty = vm_object_anon();

	return seg;
}

#if 1
segment_t * vm_segment_heap(void * p, vmobject_t * object)
{
	vmobject_heap_t * heapobject = container_of(object, vmobject_heap_t, vmobject);
	static segment_t heap = {0};
	static segment_t * seg = &heap;

	if (0 == seg->base) {
		seg->base = p;
		seg->size = heapobject->pcount << ARCH_PAGE_SIZE_LOG2;
		seg->perms = SEGMENT_P | SEGMENT_R | SEGMENT_W;
		seg->clean = 0;
		seg->read_offset = 0;
		seg->dirty = object;
	}

	return seg;
}
#endif

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
			seg->dirty = vmobject_clone(from->dirty);
		} else {
			/* Empty dirty object */
			seg->dirty = vm_object_anon();
		}
	} else {
		seg->dirty = from->dirty;
	}

	return seg;
}

vmpage_t * vm_page_steal(void * p)
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
		
		vmpage_t * vmpage = vmobject_get_page(seg->dirty, offset);
		if (vmpage) {
			vmobject_put_page(seg->dirty, offset, 0);
			vmpage_unmap(vmpage, as, p);
		} else {
			vmpage = vmpage_calloc();
		}

		return vmpage;
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
	kas_next = (void*)(p + size);
	spin_unlock(lock);

	return (void*)p;
}

void * vm_kas_get( size_t size )
{
	return vm_kas_get_aligned(size, sizeof(intptr_t));
}

static mutex_t vmpages_lock[1];

static void vmpage_finalize(void * p)
{
	vmpage_t * vmpage = p;

	for(int i=0; i<VMPAGE_MAPS; i++) {
		/* Clear any mappings for dead page */
		if (vmpage->maps[i].as && vmap_ismapped(vmpage->maps[i].as, vmpage->maps[i].p)) {
			assert(vmpage->maps[i].p != arch_get_thread()->context.stack);
			vmap_unmap(vmpage->maps[i].as, vmpage->maps[i].p);
		}
	}

#if RMAP
	MUTEX_AUTOLOCK(vmpages_lock) {
		slab_weakref_t * ref = map_putip(vmpages, vmpage->page, 0);
		if (ref) {
			vmpage_t * vmpage_check = slab_weakref_get(ref);
			assert(0 == vmpage || vmpage == vmpage_check);
		}
	}
#endif
	if (vmpage->page>0 && vmpage->flags & VMPAGE_MANAGED) {
		page_free(vmpage->page);
	}

}

vmpage_t * vmpage_alloc(vmpage_t * vmpage, page_t page)
{
	if (0 == vmpage) {
		vmpage = slab_calloc(slabvmpages);
	}
	if (page) {
		vmpage->page = page;
	} else {
		vmpage->flags = VMPAGE_MANAGED;
		vmpage->page = page_alloc();
	}
	return vmpage;
}

vmpage_t * vmpage_calloc()
{
	vmpage_t * vmpage = slab_calloc(slabvmpages);
	vmpage->flags = VMPAGE_MANAGED;
	vmpage->page = page_calloc();
	return vmpage;
}

void vmpage_map( vmpage_t * vmpage, asid as, void * p, int rw, int user )
{
#if RMAP
	MUTEX_AUTOLOCK(vmpages_lock) {
		slab_weakref_t * ref = map_putip(vmpages, vmpage->page, slab_weakref(vmpage));
		if (ref) {
			vmpage_t * vmpage_check = slab_weakref_get(ref);
			assert(0 == vmpage || vmpage == vmpage_check);
		}
	}
#endif

	/* Check if we already have this mapping */
	for(int i=0; i<VMPAGE_MAPS; i++) {
		if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
			vmap_map(as, p, vmpage->page, rw, user);
			return;
		}
	}

	/* Replace a defunct mapping, if possible */
	for(int i=0; i<VMPAGE_MAPS; i++) {
		if (!vmap_ismapped(vmpage->maps[i].as, vmpage->maps[i].p)) {
			vmpage->maps[i].as = as;
			vmpage->maps[i].p = p;
			vmap_map(as, p, vmpage->page, rw, user);
			return;
		}
	}

	/* Pick a "random" mapping */
	int replace = ((uintptr_t)as * (uintptr_t)p * 13) % VMPAGE_MAPS;
	if (vmap_ismapped(vmpage->maps[replace].as, vmpage->maps[replace].p)) {
		vmap_unmap(vmpage->maps[replace].as, vmpage->maps[replace].p);
		vmpage->maps[replace].as = as;
		vmpage->maps[replace].p = p;
		vmap_map(as, p, vmpage->page, rw, user);
	}
}

void vmpage_unmap( vmpage_t * vmpage, asid as, void * p )
{
	for(int i=0; i<VMPAGE_MAPS; i++) {
		if (vmpage->maps[i].as == as && vmpage->maps[i].p == p) {
			vmpage->maps[i].as = 0;
			vmpage->maps[i].p = 0;
			break;
		}
	}
	vmap_unmap(as, p);
}

void vmpage_setflags(vmpage_t * vmpage, int flags)
{
	vmpage->flags |= flags;
}

void vmpage_resetflags(vmpage_t * vmpage, int flags)
{
	vmpage->flags &= ~flags;
}

void vmpage_trapwrites(vmpage_t * vmpage)
{
	for(int i=0; i<VMPAGE_MAPS; i++) {
		asid as = vmpage->maps[i].as;
		void * p = vmpage->maps[i].p;

		/* Mark each mapping as read only */
		if (vmap_ismapped(as, p)) {
			//int user = vmap_isuser(as, p);
			//vmap_map(as, p, vmpage->page, 0, user);
			//vmap_unmap(as, p);
			vmpage_unmap(vmpage, as, p);
		}
	}
}

void vmpage_put_copy(vmpage_t * vmpage)
{
	vmpage_trapwrites(vmpage);
	vmpage->copies++;
}

vmpage_t * vmpage_get_copy(vmpage_t * vmpage)
{
	if (vmpage->copies)  {
		static void * src = 0;
		static void * dest = 0;
		if (0 == src) {
			src = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
			dest = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
		}

		/* Get a new page to copy into */
		vmpage_t * newpage = vmpage_alloc(0, 0);

		/* Copy the old page to the new page */
		vmap_map(kas, src, vmpage->page, 0, 0);
		vmap_map(kas, dest, newpage->page, 1, 0);
		memcpy(dest, src, ARCH_PAGE_SIZE);

		/* Remove a copy */
		vmpage->copies--;

		return newpage;
	}

	return vmpage;
}

void vmpage_release(vmpage_t * vmpage)
{
#if 0
	if (vmpage->copies)  {
		vmpage->copies--;
	} else {
		if (vmpage->flags & VMPAGE_MANAGED) {
			/* No copies, release this page */
			page_free(vmpage->page);
			vmpage->page = 0;
		}
	}
#endif
}

void vmpage_trapaccess(vmpage_t * vmpage)
{
	for(int i=0; i<VMPAGE_MAPS; i++) {
		asid as = vmpage->maps[i].as;
		void * p = vmpage->maps[i].p;

		/* Remove each mapping */
		if (vmap_ismapped(as, p)) {
			vmap_unmap(as, p);
		}
	}
}

void vmpage_age(vmpage_t * vmpage)
{
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
