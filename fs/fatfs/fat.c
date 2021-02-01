#include <sys/types.h>
#include <stdint.h>
#include "fat.h"

typedef uint8_t byte;
typedef int32_t cluster_t;
typedef int32_t sector_t;

#define FATFS_DIRENT_SIZE 32

typedef struct fatfs_t fatfs_t;
struct fatfs_t {
	fs_t fs;

	/* scratch memory */
	arena_t * arena;

	vnode_t * device;

	/* root directory */
	vnode_t * root;

	/* Free cluster extents */
	map_t * free_extents;

	/* BPB info */
	size_t sectorsize;
	size_t clustersectors;
	size_t reservedsectors;
	size_t rootentries;
	size_t sectors;
	size_t fatsize;
	size_t fatcount;

	/* BPB derived info */
	size_t clusters;
	size_t rootsectors;
	size_t clustersize;
	off64_t rootdir;
	off64_t dataarea;

	/* System area */
	void * sa;

	/* FAT(s) */
	byte * fat1;
	byte * fat2;
};

typedef struct fatfsnode_t fatfsnode_t;
struct fatfsnode_t {
	map_t * extents;

	off64_t size;

	vnode_t vnode;
};

typedef struct fatfsextent_t fatfsextent_t;
struct fatfsextent_t {
	off64_t offset;
	cluster_t cluster;
	size_t count;
};

typedef struct fatfslfn_t fatfslfn_t;
struct fatfslfn_t {
	off_t offset;
	byte checksum;
	ucs16_t lfn[256];
};

typedef int (*fatfs_dir_walk_t)(vnode_t * dir, void * p, off_t offset, byte * buf, fatfslfn_t * lfn);

#define FAT12LIMIT 4084
#define FAT16LIMIT 65524
#define DEFAULT_SECTOR_SIZE	512

static int fatx(size_t clusters)
{
	if (clusters>FAT12LIMIT) {
		if (clusters>FAT16LIMIT) {
			return 32;
		} else {
			return 16;
		}
	} else {
		return 12;
	}
}

static void fatfs_free_extent(fatfs_t * fatfs, fatfsextent_t * extent)
{
	/* Try and find the extent before */
	fatfsextent_t * prev = map_getip_cond(fatfs->free_extents, extent->cluster, MAP_LT);
	if (prev && prev->cluster + prev->count == extent->cluster) {
		/* merge into previous extent */
		prev->count += extent->count;
		extent = prev;
	}

	/* Try and find the extent after */
	fatfsextent_t * next = map_getip(fatfs->free_extents, extent->cluster + extent->count);
	if (next) {
		/* merge into next extent */
		map_removeip(fatfs->free_extents, next->cluster);
		extent->count += next->count;
	}

	/* Finally put the extent in the map */
	map_putip(fatfs->free_extents, extent->cluster, extent);
}

static fatfsextent_t * fatfs_extent(off64_t offset, cluster_t cluster, size_t count)
{
	fatfsextent_t * extent = malloc(sizeof(*extent));
	extent->offset = offset;
	extent->cluster = cluster;
	extent->count = count;
	return extent;
}

static cluster_t fatfs_cluster_next_get(fatfs_t * fatfs, cluster_t cluster)
{
	cluster_t next;

	switch(fatx(fatfs->clusters)) {
	case 12:
		if (cluster&1) {
			next = fatfs->fat1[cluster+(cluster>>1)]>>4;
			next |= (cluster_t)fatfs->fat1[cluster+(cluster>>1)+1]<<4;
		} else {
			next = fatfs->fat1[cluster+(cluster>>1)];
			next |= ((cluster_t)fatfs->fat1[cluster+(cluster>>1)+1] & 0xf)<<8;
		}
		break;
	case 16:
		next = fatfs->fat1[cluster<<1];
		next |= fatfs->fat1[1+(cluster<<1)]<<8;
		break;
	case 32:
		next = fatfs->fat1[cluster<<2];
		next |= (cluster_t)fatfs->fat1[1+(cluster<<2)]<<8;
		next |= (cluster_t)fatfs->fat1[2+(cluster<<2)]<<16;
		next |= (cluster_t)fatfs->fat1[3+(cluster<<2)]<<24;
		next &= 0xffffff;
		break;
	}

	return next;
}

#if 0
static cluster_t fatfs_cluster_next_set(fatfs_t * fatfs, cluster_t cluster, cluster_t next)
{
	switch(fatx(fatfs->clusters)) {
	case 12:
		if (cluster&1) {
			fatfs->fat1[cluster+(cluster>>1)] &= 0xf0;
			fatfs->fat1[cluster+(cluster>>1)] |= (next & 0xf);
			fatfs->fat1[cluster+(cluster>>1)+1] = next >> 4;
		} else {
			fatfs->fat1[cluster+(cluster>>1)] = (next & 0xff);
			fatfs->fat1[cluster+(cluster>>1)+1] &= 0xf0;
			fatfs->fat1[cluster+(cluster>>1)+1] |= next >> 8;
		}
		break;
	case 16:
		next = fatfs->fat1[(cluster<<1)];
		next |= fatfs->fat1[1+(cluster<<1)];
		break;
	case 32:
		next = fatfs->fat1[(cluster<<2)];
		next |= fatfs->fat1[1+(cluster<<2)];
		next |= fatfs->fat1[2+(cluster<<2)];
		next |= fatfs->fat1[3+(cluster<<2)];
		next &= 0xffffff;
		break;
	}

	return next;
}
#endif

static void fatfs_scan_fat(fatfs_t * fatfs)
{
	cluster_t start = 0;
	cluster_t cluster;
	for(cluster = 2; cluster<=fatfs->clusters+1; cluster++) {
		cluster_t next = fatfs_cluster_next_get(fatfs, cluster);
		if (next) {
			if (start) {
				fatfs_free_extent(fatfs, fatfs_extent(start, start, cluster-start));
			}
		} else {
			if (!start) {
				/* Start of a run of clusters */
				start = cluster;
			}
		}
	}
	/* Finish off last run, if any */
	if (start) {
		fatfs_free_extent(fatfs, fatfs_extent(start, start, cluster-start));
	}
}

static unsigned fatfs_field_get(const byte * sector, size_t sectorsize, int offset, size_t size)
{
	unsigned retval=0;

	check_int_bounds(offset, 0, sectorsize-size, "Read beyond end of sector");
	for(int i=0; i<size; i++) {
		retval |= sector[offset+i] << 8*i;
	}

	return retval;
}

static unsigned fatfs_bpb_get(const byte * sector, size_t sectorsize, int offset, size_t size)
{
	const int bpbbase = 0xb;

	return fatfs_field_get(sector, sectorsize, bpbbase+offset, size);
}

static fatfsextent_t * fatfs_fat_extent(fatfs_t * fatfs, off64_t offset, cluster_t start)
{
	cluster_t cluster = start;
	fatfsextent_t * extent = fatfs_extent(offset, cluster, 1);
	cluster_t next = fatfs_cluster_next_get(fatfs, cluster);
	while(next == cluster+1) {
		cluster = next;
		next = fatfs_cluster_next_get(fatfs, cluster);
		extent->count++;
	}

	return extent;
}

static int fatfs_size_directory(vnode_t * dir, void * p, off_t offset, byte * buf, fatfslfn_t * lfn)
{
	if (buf[0]) {
		return 0;
	}

	/* Last entry */
	vnode_set_size(dir, offset);

	return 1;
}

static int fatfs_walk_directory(vnode_t * dir, off_t offset, fatfs_dir_walk_t cb, void * p);
static vnode_t * fatfs_node(fatfs_t * fatfs, vnode_type type, cluster_t start, off64_t size)
{
	fatfsnode_t * node = calloc(1, sizeof(*node));
	vnode_init(&node->vnode, type, &fatfs->fs);

	if (start) {
		node->extents = splay_new(0);
		cluster_t cluster = start;
		cluster_t eof = fatfs_cluster_next_get(fatfs, 1);
		off64_t offset=0;
		do {
			fatfsextent_t * extent = fatfs_fat_extent(fatfs, offset, cluster);
			map_putip(node->extents, offset, extent);
			offset += extent->count * fatfs->clustersectors * fatfs->sectorsize;
			cluster = fatfs_cluster_next_get(fatfs, extent->cluster + extent->count - 1);
		} while(cluster != eof);

		if (size) {
			node->size = size;
		} else {
			node->size = offset;
		}
	} else {
		/* Must be the root directory */
		node->size = FATFS_DIRENT_SIZE*fatfs->rootentries;
		fatfs_walk_directory(&node->vnode, 0, fatfs_size_directory, 0);
	}

	return &node->vnode;
}

#if 0
static off64_t fatfs_cluster_offset(fatfs_t * fatfs, cluster_t cluster)
{
	return fatfs->dataarea+(cluster-2)*fatfs->clustersize;
}

static size_t fatfs_cluster_size(fatfs_t * fatfs, size_t count)
{
	return count*fatfs->clustersize;
}
#endif

static void fatfs_put_page(vnode_t * vnode, off64_t offset, vmpage_t * vmpage)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);
	fatfs_t * fatfs = container_of(vnode->fs, fatfs_t, fs);
	int writemax = ROUNDUP(node->size - offset, DEFAULT_SECTOR_SIZE);
	if (writemax > ARCH_PAGE_SIZE) {
		writemax = ARCH_PAGE_SIZE;
	}

	void * p = arena_pmap(NULL, vmpage);
	ssize_t written = 0;
	if (node->extents) {
		while(written<writemax) {
			fatfsextent_t * extent = map_getip_cond(node->extents, offset, MAP_LE);
			if (extent) {
				cluster_t skip = (offset - extent->offset)/fatfs->clustersize;
				cluster_t base = extent->cluster + skip;
				size_t towrite = (extent->count-skip)*fatfs->clustersize;
				if (towrite>(writemax-written)) {
					towrite = (writemax-written);
				}
				off64_t devoffset = fatfs->dataarea+(base-2)*fatfs->clustersize;
				written += vnode_write(fatfs->device, devoffset, p, towrite);
			} else {
				/* EOF - no more to write */
				/* FIXME: This means allocated clusters don't cover the file length */
				written = writemax;
			}
		}
	} else {
		off64_t devoffset = fatfs->rootdir+offset;
		written = vnode_write(fatfs->device, devoffset, p, writemax);
	}
}

static vmpage_t * fatfs_get_page(vnode_t * vnode, off64_t offset)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);
	fatfs_t * fatfs = container_of(vnode->fs, fatfs_t, fs);
	int readmax = ROUNDUP(node->size - offset, DEFAULT_SECTOR_SIZE);
	if (readmax > ARCH_PAGE_SIZE) {
		readmax = ARCH_PAGE_SIZE;
	}

	arena_state state = arena_getstate(NULL);
	void * p = arena_palloc(NULL, 1);
	vmpage_t * page = NULL;
	ssize_t read = 0;
	if (node->extents) {
		while(read<readmax) {
			fatfsextent_t * extent = map_getip_cond(node->extents, offset, MAP_LE);
			if (extent) {
				cluster_t skip = (offset - extent->offset)/fatfs->clustersize;
				cluster_t base = extent->cluster + skip;
				size_t toread = (extent->count-skip)*fatfs->clustersize;
				if (toread>(readmax-read)) {
					toread = (readmax-read);
				}
				off64_t devoffset = fatfs->dataarea+(base-2)*fatfs->clustersize;
				read += vnode_read(fatfs->device, devoffset, p, toread);
			} else {
				/* EOF - no more to read */
				/* FIXME: This means allocated clusters don't cover the file length */
				read = readmax;
			}
		}
		page = vm_page_steal(p);
	} else {
		off64_t devoffset = fatfs->rootdir+offset;
		read = vnode_read(fatfs->device, devoffset, p, readmax);
		if (read==readmax) {
			page = vm_page_steal(p);
		}
	}

	arena_setstate(NULL, state);

	return page;
}

static void fatfs_set_size(vnode_t * vnode, off64_t size)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);

	/* FIXME: Extend file in FAT */
	node->size = size;;
}

static off64_t fatfs_get_size(vnode_t * vnode)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);

	return node->size;
}

static int fatfs_lfn_dirent_update(fatfslfn_t * lfn, off_t offset, byte * buf)
{
	/* Skip any entries that won't be LFN entries */
	switch(buf[0]) {
	case 5:
	case 0xe5:
		/* Deleted entry */
	case '.':
		/* Dot entry */
	case 0:
		/* Last entry */
		lfn->offset = 0;
		lfn->checksum = 0;
		lfn->lfn[0] = 0;
		return 0;
	}

	cluster_t cluster = fatfs_field_get(buf, FATFS_DIRENT_SIZE, 0x1a, 2);
	off64_t size = fatfs_field_get(buf, FATFS_DIRENT_SIZE, 0x1c, 4);

	if (0==cluster && size>0 && 0xf==buf[0xb] && 0==buf[0xc]) {
		/* Update lfn as appropriate */
		int sequence = buf[0] & 0x1f;
		int last = (buf[0] & 0x60) == 0x40;
		if (last) {
			/* Ensure the lfn is terminated */
			lfn->lfn[13*(sequence+1)] = 0;
			/* Checksum */
			lfn->checksum = buf[0xd];
			/* Offset of first (last) entry */
			lfn->offset = offset;
		} else {
			/* Checksum */
			if (lfn->checksum != buf[0xd]) {
				/* Checksum mismatch */
				lfn->offset = 0;
				lfn->checksum = 0;
				lfn->lfn[0] = 0;
				return 0;
			}
		}

		/* Each LFN entry provides 13 characters */
		int lfnoffset = 13*(sequence-1);
		memcpy(lfn->lfn+lfnoffset, buf+1, 10);
		memcpy(lfn->lfn+lfnoffset+5, buf+0xe, 12);
		memcpy(lfn->lfn+lfnoffset+11, buf+0x1c, 4);
		return 1;
	}

	return 0;
}

static int fatfs_walk_directory(vnode_t * dir, off_t offset, fatfs_dir_walk_t cb, void * p)
{
	byte buf[FATFS_DIRENT_SIZE];
	off64_t next = offset;
	off64_t size = vnode_get_size(dir);
	fatfslfn_t * lfn = tmalloc(sizeof(*lfn));
	lfn->lfn[0] = 0;

	while(next<size) {
		arena_state state = arena_getstate(NULL);

		size_t read = vnode_read(dir, next, buf, countof(buf));
		if(read < countof(buf)) {
			/* Short read - not found */
			arena_setstate(NULL, state);
			return 0;
		}

		/* Check for a LFN entry */
		if (0==fatfs_lfn_dirent_update(lfn, next, buf)) {
			/* Not a LFN record */
			if (cb(dir, p, next, buf, lfn)) {
				/* Callback has signalled an end to the walk */
				arena_setstate(NULL, state);
				return 1;
			}
		}

		/* Move to next record */
		next += read;
		arena_setstate(NULL, state);
	}

	return 0;
}

typedef struct fatfs_find_file_walk_t fatfs_find_file_walk_t;
struct fatfs_find_file_walk_t {
	/* Offset of the entry in the directory */
	off_t lfnoffset;
	off_t offset;

	/* Name we're looking for */
	const char * name;

	/* Resulting vnode if found */
	vnode_t * node;
};

static char * fatfs_get_filename(byte * buf, fatfslfn_t * lfn)
{
	/* Enough room for 8.3 filename */
	char filename[8+1+3+1];

	int i;
	for(i=0; i<8; i++) {
		filename[i] = buf[i];
	}
	filename[i++] = 0;

	/* Trim trailing spaces */
	for(i=7; i>=0 && ' ' == filename[i]; i--) {
		filename[i] = 0;
	}
	if (i<0 || filename[i]) {
		i++;
	}

	if (' ' != buf[8]) {
		filename[i++] = '.';
		filename[i++] = buf[8];
		filename[i++] = (' ' == buf[9]) ? 0 : buf[9];
		filename[i++] = (' ' == buf[10]) ? 0 : buf[10];
	}

	/* Calculate checksum */
	unsigned char checksum = 0;
	for(i=0; i<11; i++) {
		checksum = ((checksum & 1) << 7) + (checksum >> 1) + buf[i];
	}

	if (lfn->checksum == checksum) {
		/* Use LFN */
		unsigned char * utf8 = tmalloc(256+128);
		utf8_from_ucs16(utf8, 256+128, lfn->lfn, 256);
		return (char*)utf8;
	}

	return tstrdup(filename);
}

static int fatfs_find_file_walk(vnode_t * dir, void * p, off_t offset, byte * buf, fatfslfn_t * lfn)
{
	/* Skip any entries that won't be LFN entries */
	switch(buf[0]) {
	case 5:
	case 0xe5:
		/* Deleted entry */
		return 0;
	case 0:
		/* Last entry */
		return 1;
	}

	fatfs_find_file_walk_t * info = (fatfs_find_file_walk_t*)p;

	char * filename = (char*)fatfs_get_filename(buf, lfn);
	if (strcmp(filename, info->name)) {
		/* No match */
		return 0;
	}

	/* Found the entry */
	fatfs_t * fatfs = container_of(dir->fs, fatfs_t, fs);
	cluster_t cluster = fatfs_field_get(buf, FATFS_DIRENT_SIZE, 0x1a, 2);
	off64_t size = fatfs_field_get(buf, FATFS_DIRENT_SIZE, 0x1c, 4);
	vnode_type directory = (buf[0xb] & 0x10) ? VNODE_DIRECTORY : VNODE_REGULAR;

	info->node = fatfs_node(fatfs, directory, cluster, size);
	info->offset = offset;
	info->lfnoffset = lfn->offset;

	return 1;
}

static vnode_t * fatfs_get_vnode(vnode_t * dir, const char * name)
{
	if ('.' == name[0] && 0 == name[1]) {
		return dir;
	} else {
		fatfs_find_file_walk_t info = { name: name };
		fatfs_walk_directory(dir, 0, fatfs_find_file_walk, &info);

		return info.node;
	}
}


typedef struct fatfs_getdents_walk_t fatfs_getdents_walk_t;
struct fatfs_getdents_walk_t {
	off_t offset;
	char * buf;
	char * next;
	size_t bufsize;
};

static int fatfs_getdents_walk(vnode_t * dir, void * p, off_t offset, byte * buf, fatfslfn_t * lfn)
{
	fatfs_getdents_walk_t * info = (fatfs_getdents_walk_t*)p;
	//info->last = offset + FATFS_DIRENT_SIZE;

	/* Skip any entries that won't be directory entries */
	switch(buf[0]) {
	case 5:
	case 0xe5:
		/* Deleted entry */
		return 0;
	case 0:
		/* Last entry */
		return 1;
	}

	size_t bufleft = info->bufsize - (info->next - info->buf);
	char * name = fatfs_get_filename(buf, lfn);
	struct dirent64 * dirent = vfs_dirent64(0, offset + FATFS_DIRENT_SIZE, name, 0);

	if (dirent->d_reclen<bufleft) {
		memcpy(info->next, dirent, dirent->d_reclen);
		info->next += dirent->d_reclen;
		return 0;
	}

	/* No room left */
	return 1;
}

static int fatfs_getdents(vnode_t * dir, off64_t offset, struct dirent64 * buf, size_t bufsize)
{
	fatfs_getdents_walk_t info = { offset, (char*)buf, (char*)buf, bufsize };
	fatfs_walk_directory(dir, offset, fatfs_getdents_walk, &info);

	return info.next - info.buf;
}

vnode_t * fatfs_open(vnode_t * dev)
{
	static vnode_ops_t vnops = {
		get_page: fatfs_get_page,
		put_page: fatfs_put_page,
		get_size: fatfs_get_size,
		set_size: fatfs_set_size,
	};
	static vfs_ops_t ops = {
		get_vnode: fatfs_get_vnode,
		getdents: fatfs_getdents,
	};

	fatfs_t * fatfs = calloc(1, sizeof(*fatfs));
	fatfs->device = dev;
	fatfs->arena = arena_get();
	fatfs->fs.vnodeops = &vnops;
        fatfs->fs.fsops = &ops;

	/* Read boot sector - Figure out where and how big FAT is */
	arena_state state = arena_getstate(fatfs->arena);
	byte * sector = arena_alloc(fatfs->arena, DEFAULT_SECTOR_SIZE);
	ssize_t read = vnode_read(fatfs->device, 0, sector, DEFAULT_SECTOR_SIZE);
	if (DEFAULT_SECTOR_SIZE!=read) {
		KTHROW(IOException, "Short read opening boot sector");
	}

	fatfs->sectorsize = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 0, 2);
	fatfs->clustersectors = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 2, 1);
	fatfs->reservedsectors = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 3, 2);
	fatfs->fatcount = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 5, 1);
	fatfs->rootentries = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 6, 2);
	fatfs->sectors = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 8, 2);
	if (0 == fatfs->sectors) {
		fatfs->sectors = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 0x20, 4);
	}
	fatfs->fatsize = fatfs_bpb_get(sector, DEFAULT_SECTOR_SIZE, 0xb, 2);

	/* Derived data */
	fatfs->rootsectors = FATFS_DIRENT_SIZE*fatfs->rootentries/fatfs->sectorsize;
	fatfs->clusters = (fatfs->sectors - fatfs->reservedsectors - fatfs->fatcount*fatfs->fatsize - fatfs->rootsectors) / fatfs->clustersectors;
	fatfs->dataarea = fatfs->sectorsize*(fatfs->reservedsectors + fatfs->fatcount*fatfs->fatsize + fatfs->rootsectors);
	fatfs->rootdir = fatfs->sectorsize*(fatfs->reservedsectors + fatfs->fatcount*fatfs->fatsize);
	fatfs->clustersize = fatfs->sectorsize*fatfs->clustersectors;

	/* Free bootsector */
	arena_setstate(fatfs->arena, state);

	/* Read in reserved sectors and fat(s) (System Area) */
	size_t sasize = fatfs->dataarea;
	fatfs->sa = arena_alloc(fatfs->arena, sasize);
	read = vnode_read(fatfs->device, 0, fatfs->sa, sasize);
	if (sasize!=read) {
		KTHROW(IOException, "Short read reading system area");
	}
	

	/* Ignore fat2 for now */
	fatfs->fat1 = fatfs->sa;
	fatfs->fat1 += fatfs->sectorsize * fatfs->reservedsectors;
	/* Scan for free clusters */
	fatfs->free_extents = splay_new(0);
	fatfs_scan_fat(fatfs);

	/* Open root directory */
	if (32==fatx(fatfs->clusters)) {
		cluster_t rootcluster = fatfs_bpb_get(fatfs->sa, sasize, 0x21, 4);
		return fatfs_node(fatfs, VNODE_DIRECTORY, rootcluster, 0);
	} else {
		return fatfs_node(fatfs, VNODE_DIRECTORY, 0, 0);
	}
}

void fatfs_test(vnode_t * dev)
{
	vnode_t * dir = fatfs_open(dev);
	if (!dir) {
		return;
	}
	vnode_t * file = vnode_get_vnode(dir, "FAT.C");
	vnode_t * lfn = vnode_get_vnode(dir, "Long name FAT entry.c");
	vnode_close(lfn);
	off64_t size = vnode_get_size(file);
	struct dirent * buf = arena_alloc(NULL, size);
	vnode_read(file, 0, buf, size);
	static char allwork[]="All work and no play makes jack a dull boy. ";
	const int allworklen = strlen(allwork);
	for(int i=0; i+allworklen<size; i+=allworklen) {
		memcpy(buf+i, allwork, allworklen);
	}
	vnode_write(file, 0, buf, size);
	vnode_sync(file);
#if 0
	int read = vfs_getdents(dir, 0, buf, size);
	for(int i=0; i<read; ) {
		struct dirent64 *dirent = buf+i;
		i += dirent->d_reclen;
	}
#endif
}
