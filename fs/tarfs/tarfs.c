#include <stddef.h>

#include "tarfs.h"

typedef struct tarfs_t tarfs_t;
typedef struct tarfs_header_t tarfs_header_t;
typedef struct tarfsnode_t tarfsnode_t;
typedef struct tarfs_dirent_t tarfs_dirent_t;

struct tarfs_t {
	map_t * vnodes;
	map_t * tree;

	dev_t * dev;

	inode_t inext;

	fs_t fs;
};


struct tarfs_header_t {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char type[1];
	char linkname[100];
	char ustar[6];
	char ustar_version[2];
	char username[32];
	char groupname[32];
	char major[8];
	char minor[8];
	char prefix[155];
};

struct tarfsnode_t {
	/* Offset in tar file */
	off_t offset;

	size_t size;
	inode_t inode;

	vnode_t vnode;
};

struct tarfs_dirent_t {
	inode_t dir;
	char * name;
};

#define TAR_BLOCKSIZE 512

/* type field */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

/* Bits used in the mode field, values in octal.  */
#define TSUID    04000          /* set UID on execution */
#define TSGID    02000          /* set GID on execution */
#define TSVTX    01000          /* reserved */
                                /* file permissions */
#define TUREAD   00400          /* read by owner */
#define TUWRITE  00200          /* write by owner */
#define TUEXEC   00100          /* execute/search by owner */
#define TGREAD   00040          /* read by group */
#define TGWRITE  00020          /* write by group */
#define TGEXEC   00010          /* execute/search by group */
#define TOREAD   00004          /* read by other */
#define TOWRITE  00002          /* write by other */
#define TOEXEC   00001          /* execute/search by other */

static uint32_t tarfs_otoi( char * cp, int size )
{
	uint32_t v = 0;

	for(int i=0; i<size && cp[i]; i++) {
		unsigned digit = cp[i] - '0';
		if (digit<8) {
			v <<= 3;
			v += (cp[i] - '0');
		}
	}

	return v;
}

static int tarfs_validate_field( const char * header, const char * value, int size )
{
	for(int i=0; i<size; i++) {
		if (header[i] != value[i]) {
			return 0;
		}
	}

	return 1;
}

static int tarfs_validate( tarfs_header_t * h )
{
	int usum = 0;
	int ssum = 0;
	int sum = 0;

	/* Extract the checksum */
	sum = tarfs_otoi(h->chksum, sizeof(h->chksum));

	/* Compute both signed and unsigned checksums */
	unsigned char * ucp = (unsigned char *)h;
	signed char * scp = (signed char *)h;
	for(int i=0; i<sizeof(*h); i++) {
		if (ucp+i>=h->chksum && ucp+i<h->type) {
			/* In checksum, just use spaces */
			usum += ' ';
			ssum += ' ';
		} else {
			usum += ucp[i];
			ssum += scp[i];
		}
	}

	if (usum != sum && ssum != sum ) {
		/* Checksum failed */
		return 0;
	}

	/* Check header format */
	if (tarfs_validate_field(h->ustar, "ustar", sizeof(h->ustar))) {
		if (tarfs_validate_field(h->ustar_version, "00", sizeof(h->ustar_version))) {
			/* POSIX.1 ustar format */
			return 1;
		}
	} else if (tarfs_validate_field(h->ustar, "ustar ", sizeof(h->ustar))) {
		if (tarfs_validate_field(h->ustar_version, " ", sizeof(h->ustar_version))) {
			/* pdtar format */
			return 1;
		}
	}

	return 1;
}

static void tarfs_readblock( tarfs_t * fs, off_t offset, void * buf )
{
	/* Round down offset */
	offset &= ~(TAR_BLOCKSIZE-1);

	/* The block read operation */
	buf_op_t op = { write: 0, p: buf, offset: offset, size: TAR_BLOCKSIZE };

	/* Submit then wait for the read to finish */
	dev_op_submit(fs->dev, &op);
	dev_op_wait(&op);
}

static char * tarfs_fullname(tarfs_header_t * h)
{
	if (h->prefix[0]) {
		char * fullname = malloc(256);
		snprintf(fullname, 255, "%s/%s", h->prefix, h->name);
		return fullname;
	} else {
		return strndup(h->name, sizeof(h->name));
	}
}

static void tarfs_add_node( tarfs_t * fs, const char * fullname, tarfsnode_t * vnode )
{
	/* Skip over any leading / */
	if ('/' == *fullname) {
		fullname++;
	}

	/* Start scanning from root */
	inode_t dnode = 1;
	char ** dirs = ssplit(dirname(tstrdup(fullname)), '/');
	for( int i=0; dirs[i]; i++ ) {
		if (*dirs[i]) {
			tarfs_dirent_t dirent = { dnode, dirs[i] };
			inode_t inode = map_getpi(fs->tree, &dirent);
			if (0 == inode) {
				/* Fake a directory */
				tarfsnode_t * dir = malloc(sizeof(*dir));
				vnode_init(&dir->vnode, VNODE_DIRECTORY, &fs->fs);
				inode = dir->inode = fs->inext++;
				tarfs_dirent_t * newdirent = malloc(sizeof(*newdirent));
				newdirent->dir = dnode;
				newdirent->name = strdup(dirs[i]);
				map_putpi(fs->tree, newdirent, dir->inode);
				map_putip(fs->vnodes, dir->inode, &dir->vnode);
			}
			dnode = inode;
		}
	}

	/* dnode is the directory, file is the new file name */
	char * file = basename(tstrdup(fullname));
	tarfs_dirent_t dirent = { dnode, file };
	inode_t inode = map_getpi(fs->tree, &dirent);
	if (inode) {
		/* File already exists, discard old one */
		map_putip(fs->vnodes, inode, &vnode->vnode);
	} else {
		/* New file, get new inode */
		tarfs_dirent_t * newdirent = malloc(sizeof(*newdirent));
		newdirent->dir = dnode;
		newdirent->name = strdup(file);
		inode = fs->inext++;

		map_putip(fs->vnodes, inode, &vnode->vnode);
		map_putpi(fs->tree, newdirent, inode);
	}
}

static void tarfs_regularfile( tarfs_t * fs, tarfs_header_t * h, off_t offset )
{
	char * fullname = tarfs_fullname(h);

	kernel_printk("Regular  : %s\n", fullname);

	tarfsnode_t * node = malloc(sizeof(*node));
	vnode_init(&node->vnode, VNODE_REGULAR, &fs->fs);
	node->offset = offset;
	node->size = tarfs_otoi(h->size, sizeof(h->size));
	tarfs_add_node(fs, fullname, node);
}

static void tarfs_directory( tarfs_t * fs, tarfs_header_t * h )
{
	char * fullname = tarfs_fullname(h);
	kernel_printk("Directory: %s\n", fullname);
}

static void tarfs_symlink( tarfs_t * fs, tarfs_header_t * h )
{
	char * fullname = tarfs_fullname(h);
	kernel_printk("Symlink  : %s\n", fullname);
}

static off_t tarfs_nextheader( tarfs_header_t * h, off_t offset )
{
	offset += TAR_BLOCKSIZE;

	if( REGTYPE == h->type[0] || AREGTYPE == h->type[0]) {
		/* Regular file */
		size_t size = tarfs_otoi(h->size, sizeof(h->size));

		size += TAR_BLOCKSIZE-1;
		size &= ~(TAR_BLOCKSIZE-1);
		offset += size;
	}

	return offset;
}

static int tarfs_dirent_cmp(void * p1, void * p2)
{
	tarfs_dirent_t * d1 = p1;
	tarfs_dirent_t * d2 = p2;

	int dir_diff = d1->dir-d2->dir;
	if (0 == dir_diff) {
		return strcmp(d1->name, d2->name);
	}

	return dir_diff;
}

static tarfs_dirent_t * tarfs_dirent_new(inode_t dir, char * name)
{
	tarfs_dirent_t * dirent = malloc(sizeof(*dirent));

	dirent->dir = dir;
	dirent->name = name;

	return dirent;
}

static void tarfs_scan( tarfs_t * fs )
{
	fs->vnodes = vector_new();
	fs->tree = tree_new(tarfs_dirent_cmp, TREE_TREAP);

	/* Root directory vnode (inode 1) */
	tarfsnode_t * root = malloc(sizeof(*root));
	vnode_init(&root->vnode, VNODE_DIRECTORY, &fs->fs);
	root->inode = 1;
	map_putip(fs->vnodes, 1, &root->vnode);

	fs->inext = 2; /* 1 is the root inode */
	off_t offset = 0;
	arena_t * arena = arena_get();
	void * buf = arena_alloc(arena, TAR_BLOCKSIZE);
	arena_state state = arena_getstate(arena);

	KTRY {
		while(1) {
			tarfs_header_t * h = buf;
			vnode_t * file = 0;
			arena_setstate(arena, state);

			/* Read the next header */
			tarfs_readblock(fs, offset, buf);

			/* Validate header */
			if (!tarfs_validate(h)) {
				break;
			}

			/* Check header type */
			switch(h->type[0]) {
			case REGTYPE:
			case AREGTYPE:
				/* Regular file */
				tarfs_regularfile(fs, h, offset);
				break;
			case DIRTYPE:
				/* Directory */
				tarfs_directory(fs, h);
				break;
			case SYMTYPE:
				/* Symbolic link */
				tarfs_symlink(fs, h);
				break;
			default:
				/* Ignore other headers */
				break;
			}

			/* Next header */
			offset = tarfs_nextheader(h, offset);
		}
	} KCATCH(DeviceException) {
		/* Failed to read next header, stop here */
	}

	arena_free(arena);
}

static vmpage_t * tarfs_get_page(vnode_t * vnode, off_t offset)
{
	tarfsnode_t * tnode = container_of(vnode, tarfsnode_t, vnode);
	tarfs_t * tfs = container_of(vnode->fs, tarfs_t, fs);
	int readmax = tnode->size - offset;
	if (readmax > ARCH_PAGE_SIZE) {
		readmax = ARCH_PAGE_SIZE;
	}

	/* Get a temporary page mapping */
	arena_t * arena = arena_thread_get();
	arena_state state = arena_getstate(arena);
	void * p = arena_palloc(arena, 1);
	char * buf = p;

	/* Copy the data into the page */
	offset += TAR_BLOCKSIZE;
	offset += tnode->offset;
	for(int i=0; i<readmax; i+=TAR_BLOCKSIZE, buf+=TAR_BLOCKSIZE) {
		tarfs_readblock(tfs, offset+i, buf);
	}

	/* Reset buf to the beginning of the page */
	buf = p;
	if (readmax<ARCH_PAGE_SIZE) {
		buf += readmax;
		memset(buf, 0, ARCH_PAGE_SIZE-readmax);
	}

	/* Steal, and return the page */
	vmpage_t * vmpage = vm_page_steal(p);
	arena_setstate(arena, state);
	return vmpage;
}

static void tarfs_put_page(vnode_t * vnode, off_t offset, page_t page)
{
	KTHROW(ReadOnlyFileException, "tarfs is read-only");
}

static vnode_t * tarfs_get_vnode(vnode_t * dir, const char * name)
{
	tarfsnode_t * tnode = container_of(dir, tarfsnode_t, vnode);
	tarfs_t * fs = container_of(dir->fs, tarfs_t, fs);
	tarfs_dirent_t dirent = { tnode->inode, (char*)name };
	inode_t inode = map_getpi(fs->tree, &dirent);

	return map_getip(fs->vnodes, inode);
}

static size_t tarfs_get_size(vnode_t * vnode)
{
	tarfsnode_t * tnode = container_of(vnode, tarfsnode_t, vnode);

	return tnode->size;
}

vnode_t * tarfs_open(dev_t * dev)
{
	static vfs_ops_t ops = {
		get_page: tarfs_get_page,
		get_vnode: tarfs_get_vnode,
		get_size: tarfs_get_size
	};

	tarfs_t * tarfs = malloc(sizeof(*tarfs));
	tarfs->dev = dev;
	tarfs->fs.fsops = &ops;
	tarfs_scan(tarfs);

	return map_getip(tarfs->vnodes, 1);
}

vnode_t * tarfs_test()
{
#if 0
	vnode_t * root = tarfs_open(dev_static(fs_tarfs_tarfs_tar, fs_tarfs_tarfs_tar_len));
	return root;
#else
	return 0;
#endif
}
