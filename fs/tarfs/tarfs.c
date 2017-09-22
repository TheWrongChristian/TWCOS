#include "tarfs.h"
#if 0
typedef struct tarfs_s {
	fs_ops_t * ops;

	map_t * vnodes;

	dev_t * dev;
} tarfs_t;


typedef struct tarfs_header_s {
	char filename[100];
	char filemode[8];
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
		v *= 8;
		v += (cp[i] - '0');
	}

	return v;
}

static int tarfs_validate( const char * header, const char * value, int size )
{
	for(int i=0; i<size; i++) {
		if (header[i] != value[i]) {
			return 0;
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

static void tarfs_regularfile( tarfs_t * fs, tarfs_header_t * h, off_t offset )
{
	size_t size = tarfs_otoi(h->size, sizeof(h->size));
	char * fullname = tarfs_fullname(tarfs_header_t * h);
	vnode_t * file = vfs_vnode_regular(fs, h, offset
}

static void tarfs_scan( tarfs_t * fs )
{
	fs->vnodes = tree_new(tree_strcmp, TREE_TREAP);
	off_t offset = 0;
	arena_t * arena = arena_get();
	void * buf = arena_alloc(arena, TAR_BLOCKSIZE);
	arena_state state = arena_getstate(arena);

	KTRY {
		while(true) {
			tarfs_header_t * h = buf;
			vnode_t * file = 0;
			arena_setstate(arena, state);

			/* Read the next header */
			tarfs_readblock(fs, offset, buf);

			/* Check it is a "ustar" header */
			if (!tarfs_validate(h->ustar, "ustar", sizeof(h->ustar))) {
				break;
			}
			if (!tarfs_validate(h->ustar_version, "00", sizeof(h->ustar_version))) {
				break;
			}

			/* FIXME: Check checksum */

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
}

static void tar_put_page(vnode_t * vnode, off_t offset, page_t page)
{
	KTHROW(ReadOnlyFileException, "tarfs is read-only");
}
#endif
