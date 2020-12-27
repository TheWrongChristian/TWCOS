#include "vfs.h"

#if INTERFACE

#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef int64_t inode_t;

struct vnode_ops_t {
	/* vnode operations */
	vmpage_t * (*get_page)(vnode_t * vnode, off64_t offset);
	void (*put_page)(vnode_t * vnode, off64_t offset, vmpage_t * page);
	void (*close)(vnode_t * vnode);
	off64_t (*get_size)(vnode_t * vnode);
	void (*set_size)(vnode_t * vnode, off64_t size);

	/* Unbuffered read/write */
	size_t (*read)(vnode_t * vnode, off64_t offset, void * buf, size_t len);
	size_t (*write)(vnode_t * vnode, off64_t offset, void * buf, size_t len);
};

struct vfs_ops_t {
	/* vnode Open/close */
	vnode_t * (*get_vnode)(vnode_t * dir, const char * name);
	void (*put_vnode)(vnode_t * dir, const char * name, vnode_t * vnode);

	/* Directory operations */
	vnode_t * (*newdir)(vnode_t * dir, const char * name);
	inode_t (*namei)(vnode_t * dir, const char * name);
	void (*link)(vnode_t * fromdir, const char * fromname, vnode_t * todir, const char * toname);
	void (*unlink)(vnode_t * dir, const char * name);
	int (*getdents)(vnode_t * dir, off64_t offset, struct dirent64 * buf, size_t bufsize);

	/* Filesystem mount/umount */
	vnode_t * (*open)(vnode_t * dev);
	void (*idle)(vnode_t * root);
};

enum vnode_type { VNODE_REGULAR, VNODE_DIRECTORY, VNODE_DEV, VNODE_FIFO, VNODE_SOCKET };

struct vnode_t {
	int ref;
	vnode_type type;
	fs_t * fs;
};

typedef struct fs_t {
	vnode_ops_t * vnodeops;
	vfs_ops_t * fsops;
};
 
#endif

#define VFS_BUFFER_LEN (64<<10)

exception_def FileException = { "FileException", &Exception };
exception_def ReadOnlyFileException = { "ReadOnlyFileException", &FileException };
exception_def UnauthorizedFileException = { "UnauthorizedFileException", &FileException };
exception_def IOException = { "IOException", &FileException };

typedef struct page_cache_key_t {
	vnode_t * vnode;
	off64_t offset;
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

static int page_cache_key_vnode_match(map_key p1, map_key p2)
{
	page_cache_key_t * k1 = (page_cache_key_t *)p1;
	page_cache_key_t * k2 = (page_cache_key_t *)p2;

	return (k1->vnode == k2->vnode);
}

static GCROOT map_t * page_cache;

void page_cache_init()
{
	INIT_ONCE();

	page_cache = tree_new(page_cache_key_comp, TREE_TREAP);
}

static void vnode_default_put_page(vnode_t * vnode, off64_t offset, vmpage_t * vmpage)
{
	KTHROW(NotImplementedException, "Not implemented");
}

static vmpage_t * vnode_default_get_page(vnode_t * vnode, off64_t offset)
{
	KTHROW(NotImplementedException, "Not implemented");
}

static void vnode_default_close(vnode_t * vnode)
{
}

off64_t vnode_default_get_size(vnode_t * vnode)
{
	return 0;
}

void vnode_default_set_size(vnode_t * vnode, off64_t size)
{
}



void vnode_fill_defaults(vnode_t * vnode)
{
	if (!vnode->fs->vnodeops->put_page) {
		vnode->fs->vnodeops->put_page = vnode_default_put_page;
	}
	if (!vnode->fs->vnodeops->get_page) {
		vnode->fs->vnodeops->get_page = vnode_default_get_page;
	}
	if (!vnode->fs->vnodeops->close) {
		vnode->fs->vnodeops->close = vnode_default_close;
	}
	if (!vnode->fs->vnodeops->get_size) {
		vnode->fs->vnodeops->get_size = vnode_default_get_size;
	}
	if (!vnode->fs->vnodeops->set_size) {
		vnode->fs->vnodeops->set_size = vnode_default_set_size;
	}
}


vmpage_t * vnode_poll_page( vnode_t * vnode, off64_t offset )
{
	offset = ROUNDDOWN(offset, ARCH_PAGE_SIZE);
	page_cache_key_t key[] = {{ vnode, offset }};

	return map_getpp(page_cache, key);
}

vmpage_t * vnode_get_page( vnode_t * vnode, off64_t offset )
{
	offset = ROUNDDOWN(offset, ARCH_PAGE_SIZE);
	page_cache_key_t key[] = {{ vnode, offset }};

	vmpage_t * vmpage = map_getpp(page_cache, key);

	if (0 == vmpage) {
		/* Not already in the cache, read it in from the FS */
		page_cache_key_t * newkey = malloc(sizeof(*newkey));
		newkey->vnode = vnode;
		newkey->offset = offset;
		vmpage = vnode->fs->vnodeops->get_page(vnode, offset);
		map_putpp(page_cache, newkey, vmpage);
	}

	return vmpage;
}

void vnode_put_page( vnode_t * vnode, off64_t offset, vmpage_t * vmpage )
{
	vnode->fs->vnodeops->put_page(vnode, offset, vmpage);
}

static void vnode_sync_page_walk(void * p, void * key, void * data)
{
	page_cache_key_t * prefix = (page_cache_key_t *)key;
	vnode_put_page(prefix->vnode, prefix->offset, (vmpage_t*)data);
}

static int vnode_prefix(page_cache_key_t * prefix, page_cache_key_t * key)
{
	return prefix->vnode == key->vnode;
}

void vnode_sync(vnode_t * vnode)
{
	page_cache_key_t prefix = {vnode: vnode, offset: 0};
	map_walkpp_prefix(page_cache, vnode_sync_page_walk, NULL, vnode_prefix, &prefix);
}

off64_t vnode_get_size(vnode_t * vnode)
{
	return vnode->fs->vnodeops->get_size(vnode);
}

void vnode_set_size(vnode_t * vnode, size_t size)
{
	return vnode->fs->vnodeops->set_size(vnode, size);
}

vnode_t * vnode_get_vnode( vnode_t * dir, const char * name)
{
	return dir->fs->fsops->get_vnode(dir, name);
}

void vnode_put_vnode( vnode_t * dir, const char * name, vnode_t * vnode)
{
	dir->fs->fsops->put_vnode(dir, name, vnode);
}

vnode_t * vnode_newdir(vnode_t * dir, const char * name)
{
	return dir->fs->fsops->newdir(dir, name);
}

vnode_t * vnode_newdir_hierarchy(vnode_t * dir, const char * name)
{
	vnode_t * v = dir;

        char ** names = path_split(name);
	for(int i=0; names[i]; i++) {
		if (*names[i]) {
			vnode_t * next = vnode_get_vnode(v, names[i]);
			if (next) {
				v = next;
			} else {
				/* Missing - make new directory */
				v = vnode_newdir(v, names[i]);
			}
		}
	}

	return v;
}

void vnode_close(vnode_t * vnode)
{
	vnode->fs->vnodeops->close(vnode);
}

typedef struct file_buffer_s {
	vnode_t * vnode;
	void * p;
	segment_t * seg;

	struct file_buffer_s * next;
} file_buffer_t;

static GCROOT file_buffer_t * bufs=0;
static int bufslock[] = {0};

static file_buffer_t * vnode_get_buffer(vnode_t * vnode, off64_t offset)
{
	file_buffer_t * buf = 0;

	SPIN_AUTOLOCK(bufslock) {
		buf = bufs;
		if (buf) {
			bufs = bufs->next;
		} else {
			/* No existing free buffers - create one */
			buf = calloc(1, sizeof(*buf));
			buf->p = vm_kas_get_aligned(VFS_BUFFER_LEN, ARCH_PAGE_SIZE);
		}
		buf->vnode = vnode;
		buf->seg = vm_segment_vnode(buf->p, VFS_BUFFER_LEN, SEGMENT_W | SEGMENT_R, vnode, ROUNDDOWN(offset, VFS_BUFFER_LEN));
		vm_kas_add(buf->seg);
	}

	return buf;
}

static void vnode_put_buffer(file_buffer_t * buf)
{
	SPIN_AUTOLOCK(bufslock) {
		vm_kas_remove(buf->seg);
		buf->vnode = 0;
		buf->seg = 0;
		buf->next = bufs;
		bufs = buf;
	}
}

static ssize_t vnode_readwrite( vnode_t * vnode, off64_t offset, void * buf, size_t len, int write)
{
	size_t processed = 0;
	char * cto = buf;

	/* Deal with file size limit */
	off64_t size = vnode_get_size(vnode);
	if (write) {
		/* Extend the file if we're going beyond EOF */
		if (offset+len > size) {
			vnode_set_size(vnode, offset+len);
		}
	} else {
		/* Clamp reads to the EOF */
		if (offset+len > size) {
			len = size-offset;
			if (len<0) {
				/* Offset is beyond the end of the file already! */
				len = 0;
			}
		}
	}

	while(processed<len) {
		// Offsets to copy from the buffer
		off64_t from = offset + processed;
		off64_t to = ROUNDDOWN(from + VFS_BUFFER_LEN, VFS_BUFFER_LEN);
		if (to>offset+len) {
			to = offset+len;
		}

		// map i
		file_buffer_t * buf = vnode_get_buffer(vnode, from);
		char * cfrom = buf->p;

		// copy to/from buf
		if (write) {
			memcpy(cfrom + (from % VFS_BUFFER_LEN), cto + processed, (to-from));
		} else {
			memcpy(cto + processed, cfrom + (from % VFS_BUFFER_LEN), (to-from));
		}

		// unmap i
		vnode_put_buffer(buf);

		// Advance cursor
		processed += (to-from);
	}

	return processed;
}

ssize_t vnode_write(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	if (vnode->fs->vnodeops->write) {
		return vnode->fs->vnodeops->write(vnode, offset, buf, len);
	} else {
		return vnode_readwrite(vnode, offset, buf, len, 1);
	}
}

ssize_t vnode_read(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	if (vnode->fs->vnodeops->read) {
		return vnode->fs->vnodeops->read(vnode, offset, buf, len);
	} else {
		return vnode_readwrite(vnode, offset, buf, len, 0);
	}
}

void fs_idle(vnode_t * root)
{
	root->fs->fsops->idle(root);
}

vnode_t * vnode_init(vnode_t * vnode, vnode_type type, fs_t * fs)
{
	vnode->type = type;
	vnode->fs = fs;
	vnode->ref = 1;

	return vnode;
}


/*
 * Virtual filesystem tree
 */

typedef struct vfstree_t vfstree_t;
struct vfstree_t {
	map_t * tree;

	vnode_t root;
	fs_t fs;
};

typedef struct vfstree_dirent_t vfstree_dirent_t;
struct vfstree_dirent_t {
	vnode_t * dir;
	const char * name;
};

vnode_t * vfstree_get_vnode(vnode_t * dir, const char * name) 
{
	vfstree_t * vfstree = container_of(dir->fs, vfstree_t, fs);
	vnode_t * vnode = 0;
	vfstree_dirent_t key = { dir, name };

	return map_getpp(vfstree->tree, &key);
}

void vfstree_put_vnode(vnode_t * dir, const char * name, vnode_t * vnode) 
{
	vfstree_t * vfstree = container_of(dir->fs, vfstree_t, fs);
	vfstree_dirent_t * key = malloc(sizeof(*key));
	key->dir = dir;
	key->name = strdup(name);
	map_putpp(vfstree->tree, key, vnode);
}

static int vfstree_dirent_cmp(void * p1, void * p2)
{
	vfstree_dirent_t * d1 = p1;
	vfstree_dirent_t * d2 = p2;

	int dir_diff = d1->dir-d2->dir;
	if (0 == dir_diff) {
		if (0 == d1->name && d2->name) {
			return -1;
		} else if (0 == d1->name && d2->name) {
			return 1;
		} else {
			return strcmp(d1->name, d2->name);
		}
	}

	return dir_diff;
}

typedef uint8_t byte;
typedef struct vfstree_getdents_walk_t vfstree_getdents_walk_t;
struct vfstree_getdents_walk_t {
	off64_t startoffset;
	off64_t offset;
	byte * buf;
	byte * next;
	size_t bufsize;
};

static int vfstree_getdents_walk(void * p, void * key, void * data)
{
	vfstree_getdents_walk_t * info = (vfstree_getdents_walk_t*)p;

	vfstree_dirent_t * vfstreedirent = (vfstree_dirent_t*)key;
	ino64_t inode = (uintptr_t)data;
	struct dirent64 * dirent = vfs_dirent64(inode, info->offset, vfstreedirent->name, 0);
	if (dirent->d_off < info->startoffset) {
		/* Already read this entry */
		info->offset += dirent->d_reclen;
		return 0;
	}

	size_t bufleft = info->bufsize - (info->next - info->buf);

	if (dirent->d_reclen<bufleft) {
		/* New entry with sufficient space */
		memcpy(info->next, dirent, dirent->d_reclen);
		info->next += dirent->d_reclen;
		info->offset += dirent->d_reclen;
		return 0;
	}

	/* No room left */
	return 1;
}

vnode_t * vfstree_newdir(vnode_t * dir, const char * name)
{
	vfstree_t * vfstree = container_of(dir->fs, vfstree_t, fs);
	vnode_t * newdir = malloc(sizeof(*newdir));
	vnode_put_vnode(dir, name, vnode_init(newdir, VNODE_DIRECTORY, &vfstree->fs));

	return newdir;
}

static int vfstree_getdents_prefix(void * prefix, void * key)
{
	vfstree_dirent_t * dirent = (vfstree_dirent_t*)key;
	vfstree_dirent_t * prefixdirent = (vfstree_dirent_t*)prefix;

	return dirent->dir == prefixdirent->dir;
}

static int vfstree_getdents(vnode_t * dir, off64_t offset, struct dirent64 * buf, size_t bufsize)
{
	vfstree_t * fs = container_of(dir->fs, vfstree_t, fs);
	vfstree_dirent_t prefix = { dir };
	vfstree_getdents_walk_t info = { offset, 0, buf, buf, bufsize };

	map_walkpi_prefix(fs->tree, vfstree_getdents_walk, &info, vfstree_getdents_prefix, &prefix);

	return info.next - info.buf;
}

vnode_t * vfstree_new()
{
	static vfs_ops_t ops = {
		.newdir = vfstree_newdir,
		.get_vnode = vfstree_get_vnode,
		.put_vnode = vfstree_put_vnode,
		.getdents = vfstree_getdents,
        };
	vfstree_t * vfstree = malloc(sizeof(*vfstree));
	vfstree->tree = treap_new(vfstree_dirent_cmp);
	vfstree->fs.fsops = &ops;
	return vnode_init(&vfstree->root, VNODE_DIRECTORY, &vfstree->fs);
}

static GCROOT map_t * mounts=0;
static rwlock_t mountslock[]={0};

void vfs_mount(vnode_t * dir, vnode_t * root)
{
	WRITER_AUTOLOCK(mountslock) {
		if (0==mounts) {
			mounts=splay_new(0);
		}

		vnode_t * existing = map_putpp(mounts, dir, root);
	}
}

void vfs_umount(vnode_t * dir)
{
	WRITER_AUTOLOCK(mountslock) {
		if (0==mounts) {
			mounts=splay_new(0);
		}

		map_removepp(mounts, dir);
	}
}

vnode_t * vfs_reparse(vnode_t * dir)
{
	vnode_t * out=0;
	READER_AUTOLOCK(mountslock) {
		if (0==mounts) {
			rwlock_escalate(mountslock);
			mounts=splay_new(0);
		}

		out = map_getpp(mounts, dir);
	}

	return out ? out : dir;
}

void * vfs_dirent64(ino64_t ino, ino64_t offset, const char * name, char type)
{
	struct dirent64 * dirent;
	size_t reclen = ROUNDUP(sizeof(*dirent) + strlen(name) + 1 + 1, sizeof(dirent->d_ino));

	dirent = tmalloc(reclen);
	dirent->d_ino = ino;
	dirent->d_off = offset;
	dirent->d_reclen = reclen;
	strcpy(dirent->d_name, name);

	return dirent;
}

int vfs_getdents(vnode_t * dir, off64_t offset, struct dirent64 * buf, size_t bufsize)
{
	return (dir->fs->fsops->getdents) ? dir->fs->fsops->getdents(dir, offset, buf, bufsize) : 0;
}

void vfs_test(vnode_t * root)
{
	if (0 == root) {
		kernel_panic("root is null!");
	}
#if 0
	/* Open user/shell/init */
	vnode_t * user = vnode_get_vnode(root, "user");
	if (0 == user) {
		kernel_panic("user is null");
	}

	vnode_t * shell = vnode_get_vnode(user, "shell");
	if (0 == shell) {
		kernel_panic("shell is null");
	}

	vnode_t * init = vnode_get_vnode(shell, "init");
#endif
	vnode_t * init = file_namev("/sbin/init");
	if (0 == init) {
		kernel_panic("init is null");
	}

	struct Elf32_Ehdr * p = (struct Elf32_Ehdr *)0x00010000;
	segment_t * seg = vm_segment_vnode(p, vnode_get_size(init), SEGMENT_U | SEGMENT_P, init, 0);
	map_putpp(arch_get_thread()->process->as, p, seg);
	int supported = elf_check_supported(p);
	kernel_printk("Supported: %d\n", supported);
}
