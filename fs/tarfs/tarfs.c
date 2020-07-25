#include <stddef.h>
#include <sys/types.h>

#include "tarfs.h"


typedef struct tarfs_header_t tarfs_header_t;
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

typedef struct tarfsnode_t tarfsnode_t;
struct tarfsnode_t {
	/* Offset in tar file */
	dev_t * dev;
	off64_t offset;

	off64_t size;

	vnode_t vnode;
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
		if (ucp+i>=(unsigned char *)h->chksum && ucp+i<(unsigned char *)h->type) {
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

static void tarfs_readblock( dev_t * dev, off64_t offset, void * buf )
{
	/* Round down offset */
	offset &= ~(TAR_BLOCKSIZE-1);

	/* The block read operation */
	buf_op_t op = { .write = 0, .p = buf, .offset = offset, .size = TAR_BLOCKSIZE };

	/* Submit then wait for the read to finish */
	dev_op_submit(dev, &op);
	dev_op_wait(&op);
}

static char * tarfs_fullname(tarfs_header_t * h)
{
	if (h->prefix[0]) {
		char * fullname = tmalloc(256);
		snprintf(fullname, 255, "%s/%s", h->prefix, h->name);
		return fullname;
	} else {
		return tstrndup(h->name, sizeof(h->name));
	}
}

static void tarfs_add_node( vnode_t * root, const char * fullname, vnode_t * vnode )
{
	/* Skip over any leading / */
	while('/' == *fullname) {
		fullname++;
	}

	vnode_t * v = root;
	const char * file;
	const char * dir;

	if (vnode) {
		dir = dirname(tstrdup(fullname));
		file = basename(tstrdup(fullname));
	} else {
		dir = fullname;
		file = 0;
	}

	v = vnode_newdir_hierarchy(v, dir);
	if (file && v) {
		vnode_put_vnode(v, file, vnode);
	}
}

static vmpage_t * tarfs_get_page(vnode_t * vnode, off64_t offset);
static void tarfs_put_page(vnode_t * vnode, off64_t offset, vmpage_t * page);
static off64_t tarfs_get_size(vnode_t * vnode);
static void tarfs_set_size(vnode_t * vnode, off64_t size);

static void tarfs_regularfile( vnode_t * root, tarfs_header_t * h, dev_t * dev, off64_t offset )
{
	static vnode_ops_t vnops = {
		.get_page = tarfs_get_page,
		.put_page = tarfs_put_page,
		.get_size = tarfs_get_size,
		.set_size = tarfs_set_size,
	};
	static fs_t fs = {
		.vnodeops = &vnops
	};

	char * fullname = tarfs_fullname(h);

	kernel_printk("Regular  : %s\n", fullname);

	tarfsnode_t * node = malloc(sizeof(*node));
	vnode_init(&node->vnode, VNODE_REGULAR, &fs);
	node->dev = dev;
	node->offset = offset;
	node->size = tarfs_otoi(h->size, sizeof(h->size));
	tarfs_add_node(root, fullname, &node->vnode);
}

static void tarfs_directory( vnode_t * root, tarfs_header_t * h )
{
	char * fullname = tarfs_fullname(h);
	kernel_printk("Directory: %s\n", fullname);

	tarfs_add_node(root, fullname, NULL);
}

static void tarfs_symlink( vnode_t * root, tarfs_header_t * h )
{
	char * fullname = tarfs_fullname(h);
	kernel_printk("Symlink  : %s\n", fullname);
}

static off64_t tarfs_nextheader( tarfs_header_t * h, off64_t offset )
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

static void tarfs_scan( vnode_t * root, dev_t * dev )
{
	off64_t offset = 0;
	void * buf = arena_alloc(NULL, TAR_BLOCKSIZE);

	KTRY {
		while(1) {
			tarfs_header_t * h = buf;

			/* Read the next header */
			tarfs_readblock(dev, offset, buf);

			/* Validate header */
			if (!tarfs_validate(h)) {
				break;
			}

			ARENA_AUTOSTATE(NULL) {
				/* Check header type */
				switch(h->type[0]) {
				case REGTYPE:
				case AREGTYPE:
					/* Regular file */
					tarfs_regularfile(root, h, dev, offset);
					break;
				case DIRTYPE:
					/* Directory */
					tarfs_directory(root, h);
					break;
				case SYMTYPE:
					/* Symbolic link */
					tarfs_symlink(root, h);
					break;
				default:
					/* Ignore other headers */
					break;
				}

				/* Next header */
				offset = tarfs_nextheader(h, offset);
			}
		}
	} KCATCH(DeviceException) {
		/* Failed to read next header, stop here */
	}
}

static vmpage_t * tarfs_get_page(vnode_t * vnode, off64_t offset)
{
	tarfsnode_t * tnode = container_of(vnode, tarfsnode_t, vnode);
	int readmax = tnode->size - offset;
	if (readmax > ARCH_PAGE_SIZE) {
		readmax = ARCH_PAGE_SIZE;
	}

	/* Get a temporary page mapping */
	arena_state state = arena_getstate(NULL);
	void * p = arena_palloc(NULL, 1);
	char * buf = p;

	/* Copy the data into the page */
	offset += TAR_BLOCKSIZE;
	offset += tnode->offset;
	for(int i=0; i<readmax; i+=TAR_BLOCKSIZE, buf+=TAR_BLOCKSIZE) {
		tarfs_readblock(tnode->dev, offset+i, buf);
	}

	/* Reset buf to the beginning of the page */
	buf = p;
	if (readmax<ARCH_PAGE_SIZE) {
		buf += readmax;
		memset(buf, 0, ARCH_PAGE_SIZE-readmax);
	}

	/* Steal, and return the page */
	vmpage_t * vmpage = vm_page_steal(p);
	arena_setstate(NULL, state);
	return vmpage;
}

static void tarfs_put_page(vnode_t * vnode, off64_t offset, vmpage_t * page)
{
	KTHROW(ReadOnlyFileException, "tarfs is read-only");
}

static off64_t tarfs_get_size(vnode_t * vnode)
{
	tarfsnode_t * tnode = container_of(vnode, tarfsnode_t, vnode);

	return tnode->size;
}

static void tarfs_set_size(vnode_t * vnode, off64_t size)
{
	KTHROW(ReadOnlyFileException, "tarfs is read-only");
}

vnode_t * tarfs_open(dev_t * dev)
{
	vnode_t * root = vfstree_new();
	ARENA_AUTOSTATE(NULL) {
		tarfs_scan(root, dev);
 	}

	return root;
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
