#include <stddef.h>

#include "tarfs.h"

typedef struct tarfs_s {
	fs_ops_t * ops;

	map_t * vnodes;

	dev_t * dev;
} tarfs_t;


typedef struct tarfs_header_s {
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
} tarfs_header_t;

typedef struct tarfsnode_s {
	tarfs_t * fs;
	off_t offset;
} tarfsnode_t;

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
	unsigned char * ucp = h;
	signed char * scp = h;
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

static void tarfs_regularfile( tarfs_t * fs, tarfs_header_t * h, off_t offset )
{
	size_t size = tarfs_otoi(h->size, sizeof(h->size));
	char * fullname = tarfs_fullname(h);
	kernel_printk("Regular  : %s\n", fullname);
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

		size += TAR_BLOCKSIZE;
		size &= ~(TAR_BLOCKSIZE-1);
		offset += size;
	}

	return offset;
}

static void tarfs_scan( tarfs_t * fs )
{
	fs->vnodes = tree_new(map_strcmp, TREE_TREAP);
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

static page_t tar_get_page(vnode_t * vnode, off_t offset)
{
	return 0;
}

static void tar_put_page(vnode_t * vnode, off_t offset, page_t page)
{
	KTHROW(ReadOnlyFileException, "tarfs is read-only");
}


void tarfs_test()
{
	tarfs_t * tarfs = malloc(sizeof(*tarfs));
	tarfs->dev = dev_static(fs_tarfs_tarfs_tar, fs_tarfs_tarfs_tar_len);
	tarfs_scan(tarfs);
}
