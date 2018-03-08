#include "vfs.h"

#if INTERFACE

#include <stdint.h>
#include <stddef.h>
#include <stdint.h>

typedef int64_t inode_t;

struct vfs_ops_t {
	/* vnode operations */
	page_t (*get_page)(vnode_t * vnode, off_t offset);
	void (*put_page)(vnode_t * vnode, off_t offset, page_t page);
	void (*close)(vnode_t * vnode);
	size_t (*get_size)(vnode_t * vnode);

	/* vnode Open/close */
	vnode_t * (*get_vnode)(vnode_t * dir, const char * name);
	void (*put_vnode)(vnode_t * vnode);

	/* Directory operations */
	inode_t (*namei)(vnode_t * dir, const char * name);
	void (*link)(vnode_t * fromdir, const char * fromname, vnode_t * todir, const char * toname);
	void (*unlink)(vnode_t * dir, const char * name);

	/* Filesystem mount/umount */
	vnode_t * (*open)(vnode_t * dev);
	void (*idle)(fs_t * fs);
};

enum vnode_type { VNODE_REGULAR, VNODE_DIRECTORY, VNODE_DEV, VNODE_FIFO, VNODE_SOCKET };

struct vnode_t {
	int ref;
	vnode_type type;
	fs_t * fs;
};

typedef struct fs_t {
	vfs_ops_t * fsops;
};
 
#endif

exception_def FileException = { "FileException", &Exception };
exception_def ReadOnlyFileException = { "ReadOnlyFileException", &FileException };
exception_def UnauthorizedFileException = { "UnauthorizedFileException", &FileException };
exception_def IOException = { "IOException", &FileException };

typedef struct page_cache_key_t {
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



page_t vnode_get_page( vnode_t * vnode, off_t offset )
{
	page_cache_key_t key[] = {{ vnode, offset }};

	page_t page = map_getpi(page_cache, key);

	if (0 == page) {
		/* Not already in the cache, read it in from the FS */
		page_cache_key_t * newkey = malloc(sizeof(*newkey));
		newkey->vnode = vnode;
		newkey->offset = offset;
		page = vnode->fs->fsops->get_page(vnode, offset);
		map_putpi(page_cache, newkey, page);
	}

	return page;
}

void vnode_put_page( vnode_t * vnode, off_t offset, page_t page )
{
	vnode->fs->fsops->put_page(vnode, offset, page);
}

size_t vnode_get_size(vnode_t * vnode)
{
	vnode->fs->fsops->get_size(vnode);
}

vnode_t * vnode_get_vnode( vnode_t * dir, const char * name )
{
	return dir->fs->fsops->get_vnode(dir, name);
}

void vnode_close(vnode_t * vnode)
{
	vnode->fs->fsops->close(vnode);
}

void fs_idle(vnode_t * root)
{
	root->fs->fsops->idle(root->fs);
}

void vnode_init(vnode_t * vnode, vnode_type type, fs_t * fs)
{
	vnode->type = type;
	vnode->fs = fs;
	vnode->ref = 1;
}


void vfs_test(vnode_t * root)
{
	/* Open libk/tree.c */
	vnode_t * libk = vnode_get_vnode(root, "libk");
	vnode_t * tree_c = vnode_get_vnode(libk, "tree.c");

	char * p = 0x00010000;
	segment_t * seg = vm_segment_vnode(p, vnode_get_size(tree_c), SEGMENT_U | SEGMENT_P, tree_c, 0);
	map_putpp(arch_get_thread()->as, p, seg);
	kernel_printk("test.c:\n%s", p);
}
