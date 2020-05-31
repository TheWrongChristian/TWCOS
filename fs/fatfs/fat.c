#include "fat.h"
#include <stdint.h>

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
	off_t rootdir;
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
	off_t offset;
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

static fatfsextent_t * fatfs_extent(off_t offset, cluster_t cluster, size_t count)
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

static fatfsextent_t * fatfs_fat_extent(fatfs_t * fatfs, off_t offset, cluster_t start)
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
	}

	return &node->vnode;
}

static vmpage_t * fatfs_get_page(vnode_t * vnode, off_t offset)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);
	fatfs_t * fatfs = container_of(vnode->fs, fatfs_t, fs);
	int readmax = node->size - offset;
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
				read += vnode_read(fatfs->device, fatfs->dataarea+(base-2)*fatfs->clustersize, p, toread);
			} else {
				/* EOF - no more to read */
				/* FIXME: This means allocated clusters don't cover the file length */
				read = readmax;
			}
		}
		page = vm_page_steal(p);
	} else {
		off_t devoffset = fatfs->rootdir+offset;
		read = vnode_read(fatfs->device, fatfs->rootdir+offset, p, readmax);
		if (read==readmax) {
			page = vm_page_steal(p);
		}
	}

	arena_setstate(NULL, state);

	return page;
}

static off_t fatfs_get_size(vnode_t * vnode)
{
	fatfsnode_t * node = container_of(vnode, fatfsnode_t, vnode);

	return node->size;
}

static int fatfs_cluster_to_sectors(fatfs_t * fatfs, cluster_t cluster, sector_t * start, size_t * count)
{

}

static vnode_t * fatfs_get_vnode(vnode_t * dir, const char * name)
{
	fatfsnode_t * node = container_of(dir, fatfsnode_t, vnode);
	fatfs_t * fatfs = container_of(dir->fs, fatfs_t, fs);
	byte buf[FATFS_DIRENT_SIZE];
	int dot = ('.' == name[0] && 0 == name[1]);
	int dotdot = ('.' == name[0] && '.' == name[1] && 0 == name[2]);
	off_t next = 0;

	while(1) {
		size_t read = vnode_read(dir, next, buf, countof(buf));
		/* Enough room for 8.3 filename */
		char filename[8+1];
		char ext[3+1];
	
		if(read < countof(buf)) {
			/* Short read - not found */
			return 0;
		}
		next += read;

		switch(buf[0]) {
		case 0:
			/* Last entry - not found */
			return 0;
		case 5:
		case 0xe5:
			/* Deleted entry, move onto next entry */
			continue;
		case '.':
			/* Dot entry */
			if (dot && ' ' == buf[1]) {
				/* Current directory */
				return dir;
			} else if (dotdot && '.' == buf[1] && ' ' == buf[2]) {
				/* Parent directory */
				return 0;
			}
			continue;
		default:
			for(int i=0; i<8; i++) {
				filename[i] = buf[i];
			}
			filename[8] = 0;
			/* Trim trailing spaces */
			for(int i=7; i>=0 && ' ' == filename[i]; i--) {
				filename[i] = 0;
			}
			for(int i=0; i<3; i++) {
				ext[i] = buf[i+8];
			}
			ext[3] = 0;
			/* Trim trailing spaces */
			for(int i=2; i>=0 && ' ' == ext[i]; i--) {
				ext[i] = 0;
			}
			if (ext[0]) {
				int len=strlen(filename);
				filename[len++] = '.';
				for(int i=0; i<4; i++) {
					filename[len+i] = ext[i];
				}
			}
			if (strcmp(filename, name)) {
				/* No match */
				continue;
			}

			break;
		}

		/* Found an entry */
		cluster_t cluster = fatfs_field_get(buf, countof(buf), 0x1a, 2);
		off_t size = fatfs_field_get(buf, countof(buf), 0x1c, 4);
		vnode_type directory = (buf[0xb] & 0x10) ? VNODE_DIRECTORY : VNODE_REGULAR;
		return fatfs_node(fatfs, directory, cluster, size);
	}

	return 0;
}


vnode_t * fatfs_open(dev_t * dev)
{
	static vnode_ops_t vnops = {
		get_page: fatfs_get_page,
		get_size: fatfs_get_size,
	};
	static vfs_ops_t ops = {
		get_vnode: fatfs_get_vnode,
	};

	fatfs_t * fatfs = calloc(1, sizeof(*fatfs));
	fatfs->device = dev_vnode(dev);
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

void fatfs_test(dev_t * dev)
{
	vnode_t * root = fatfs_open(dev);
}
