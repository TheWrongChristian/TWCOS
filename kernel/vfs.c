#include "vfs.h"

#if INTERFACE

#include <stddef.h>

typedef struct vnode_ops_s {
	page_t (*get_page)( vnode_s * vnode, off_t offset);
	void (*put_page)( vnode_s * vnode, off_t offset, page_t page );
	void (*close)( vnode_s * vnode );
} vnode_ops_t;

typedef struct fs_ops_s {
	void (*close)(fs_t * fs);
	vnode_t * (*get_vnode)(vnode_t * dir, const char * name);
	vnode_t * (*get_root_vnode)();
} fs_ops_t;

enum vnode_type { VNODE_REGULAR, VNODE_DIRECTORY, VNODE_DEV, VNODE_FIFO, VNODE_SOCKET };

typedef struct vnode_s {
	struct fs_s * fs;
	int ref;

	vnode_type type;
	union {
		struct {
			size_t size;

			void * fsnode;
		} regular;
		struct {
			struct vnode_s * parent;
			void * fsnode;
		} dir;
	} u;
} vnode_t;

typedef struct fs_s {
	vnode_ops_s * vnops;
	fs_ops_s * fsops;
} fs_t;

#endif

exception_def FileException = { "FileException", &Exception };
exception_def ReadOnlyFileException = { "ReadOnlyFileException", &FileException };
exception_def UnauthorizedFileException = { "UnauthorizedFileException", &FileException };
exception_def IOException = { "IOException", &FileException };

typedef struct page_cache_key_s {
	vnode_t * vnode;
	off_t offset;
} page_cache_key_t;

static int page_cache_key_comp( map_key p1, map_key p2 )
{
	page_cache_key_t * k1 = (page_cache_key_t *)p1;
	page_cache_key_t * k2 = (page_cache_key_t *)p2;

	if (k1->vnode == k2->vnode) {
		/* Same vnode, sort by offset */
		return (k1->offset < k2->offset) ? 
			-1 : (k1->offset > k2->offset) ?
			1 : 0;
	} else {
		/* Sort by vnode pointer */
		return (k1->vnode < k2->vnode) ? 
			-1 : (k1->vnode > k2->vnode) ?
			1 : 0;
	}
}

static map_t * page_cache;

void page_cache_init()
{
	INIT_ONCE();

	page_cache = tree_new(page_cache_key_comp, TREE_TREAP);
}



page_t vfs_get_page( vnode_t * vnode, off_t offset )
{
	page_cache_key_t key[] = {{ vnode, offset }};

	page_t page = map_getpi(page_cache, key);

	if (0 == page) {
		/* Not already in the cache, read it in from the FS */
		page_cache_key_t * newkey = malloc(sizeof(*newkey));
		newkey->vnode = vnode;
		newkey->offset = offset;
		page = vnode->fs->vnops->get_page(vnode, offset);
		map_putpi(page_cache, newkey, page);
	}

	return page;
}

void vfs_put_page( vnode_t * vnode, off_t offset, page_t page )
{
	vnode->fs->vnops->put_page(vnode, offset, page);
}

vnode_t * vfs_get_vnode( vnode_t * dir, const char * name )
{
	return dir->fs->fsops->get_vnode(dir, name);
}

void vfs_vnode_close(vnode_t * vnode)
{
	vnode->fs->vnops->close(vnode);
}

void vfs_fs_close(vnode_t * root)
{
	root->fs->fsops->close(root->fs);
}

static void vnode_mark(void * p)
{
	vnode_t * vnode = p;
	slab_gc_mark(vnode->fs);
}

static void vnode_finalize(void * p)
{
}

static slab_type_t vnodes[1] = {SLAB_TYPE(sizeof(vnodes), vnode_mark, vnode_finalize)};

static vnode_t * vfs_vnode_alloc(fs_t * fs, vnode_type type)
{
	vnode_t * vnode = slab_alloc(vnodes);

	vnode->type = type;
	/* vnode->ops = (fs) ? fs->vnops : 0; */
	vnode->fs = fs;

	return vnode;
}

vnode_t * vfs_vnode_directory(fs_t * fs, void * fsnode)
{
	return vfs_vnode_alloc(fs, VNODE_DIRECTORY);
}

vnode_t * vfs_vnode_regular(fs_t * fs, void * fsnode)
{
	return vfs_vnode_alloc(fs, VNODE_REGULAR);
}
