#include "vfs.h"

#if INTERFACE

#include <stdint.h>
#include <stddef.h>
#include <stdint.h>

typedef int64_t inode_t;

#if 0
typedef struct vnode_ops_s vnode_ops_t;
typedef struct vnode_s vnode_t;
typedef struct fs_ops_s fs_ops_t;
typedef struct fs_s fs_t;
#endif

struct vnode_ops_t {
	fs_t * (*get_fs)(vnode_t * vnode);
	page_t (*get_page)( vnode_t * vnode, off_t offset);
	void (*put_page)( vnode_t * vnode, off_t offset, page_t page );
	inode_t (*namei)(vnode_t * dir, const char * name);
	void (*close)( vnode_t * vnode );
};

struct fs_ops_t {
	void (*close)(fs_t * fs);
	vnode_t * (*get_vnode)(vnode_t * dir, const char * name);
	void (*put_vnode)(vnode_t * vnode);
	vnode_t * (*open)(vnode_t * dev);
};

enum vnode_type { VNODE_REGULAR, VNODE_DIRECTORY, VNODE_DEV, VNODE_FIFO, VNODE_SOCKET };

struct vnode_t {
	vnode_ops_t * vnops;
	fs_t * fs;
	int ref;
};

typedef struct fs_t {
	fs_ops_t * fsops;
};
 
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
		page = vnode->vnops->get_page(vnode, offset);
		map_putpi(page_cache, newkey, page);
	}

	return page;
}

void vfs_put_page( vnode_t * vnode, off_t offset, page_t page )
{
	vnode->vnops->put_page(vnode, offset, page);
}

vnode_t * vfs_get_vnode( vnode_t * dir, const char * name )
{
	return dir->fs->fsops->get_vnode(dir, name);
}

void vfs_vnode_close(vnode_t * vnode)
{
	vnode->vnops->close(vnode);
}

void vfs_fs_close(vnode_t * root)
{
	root->fs->fsops->close(root->fs);
}
