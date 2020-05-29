#include "fat.h"
#include <stdint.h>

typedef uint8_t byte;
typedef int32_t cluster_t;

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
	off_t dataarea;

	/* System area */
	void * sa;

	/* FAT(s) */
	byte * fat1;
	byte * fat2;
};

typedef struct fatfsnode_t fatfsnode_t;
struct fatfsnode_t {
	map_t * extents;

	off_t size;

	vnode_t vnode;
};

typedef struct fatfsextent_t fatfsextent_t;
struct fatfsextent_t {
	cluster_t cluster;
	size_t count;
};

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

static fatfsextent_t * fatfs_extent(cluster_t cluster, size_t count)
{
	fatfsextent_t * extent = malloc(sizeof(*extent));
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

static void fatfs_scan_fat(fatfs_t * fatfs)
{
	cluster_t start = 0;
	cluster_t cluster;
	for(cluster = 2; cluster<=fatfs->clusters+1; cluster++) {
		cluster_t next = fatfs_cluster_next_get(fatfs, cluster);
		if (next) {
			if (start) {
				fatfs_free_extent(fatfs, fatfs_extent(start, cluster-start));
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
		fatfs_free_extent(fatfs, fatfs_extent(start, cluster-start));
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

static fatfsextent_t * fatfs_fat_extent(fatfs_t * fatfs, cluster_t start)
{
	cluster_t cluster = start;
	fatfsextent_t * extent = fatfs_extent(cluster, 1);
	cluster_t next = fatfs_cluster_next_get(fatfs, cluster);
	while(next == cluster+1) {
		cluster = next;
		next = fatfs_cluster_next_get(fatfs, cluster);
		extent->count++;
	}

	return extent;
}

static vnode_t * fatfs_node(fatfs_t * fatfs, vnode_type type, cluster_t start, off_t size)
{
	fatfsnode_t * node = calloc(1, sizeof(*node));
	vnode_init(&node->vnode, type, &fatfs->fs);

	if (start) {
		node->extents = splay_new(0);
		cluster_t cluster = start;
		cluster_t eof = fatfs_cluster_next_get(fatfs, 1);
		off_t offset=0;
		do {
			fatfsextent_t * extent = fatfs_fat_extent(fatfs, cluster);
			map_putip(node->extents, offset, extent);
			offset += extent->count * fatfs->clustersectors * fatfs->sectorsize;
			cluster = fatfs_cluster_next_get(fatfs, extent->cluster + extent->count - 1);
		} while(cluster != eof);
	} else {
		/* Must be the root directory */
	}

	return &node->vnode;
}

vnode_t * fatfs_open(dev_t * dev)
{
	fatfs_t * fatfs = calloc(1, sizeof(*fatfs));
	fatfs->device = dev_vnode(dev);
	fatfs->arena = arena_get();

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
	fatfs->rootsectors = 32*fatfs->rootentries/fatfs->sectorsize;
	fatfs->clusters = (fatfs->sectors - fatfs->reservedsectors - fatfs->fatcount*fatfs->fatsize - fatfs->rootsectors) / fatfs->clustersectors;
	fatfs->dataarea = fatfs->reservedsectors + fatfs->fatcount*fatfs->fatsize + fatfs->rootsectors;

	/* Free bootsector */
	arena_setstate(fatfs->arena, state);

	/* Read in reserved sectors and fat(s) (System Area) */
	size_t sasize = fatfs->sectorsize*fatfs->dataarea;
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
		vnode_t * vnode = fatfs_node(fatfs, VNODE_REGULAR, 2, 0);
		return fatfs_node(fatfs, VNODE_DIRECTORY, 0, 0);
	}
}

void fatfs_test(dev_t * dev)
{
	vnode_t * root = fatfs_open(dev);
}
