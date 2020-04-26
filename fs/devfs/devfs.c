#include "devfs.h"

typedef struct devfs_t devfs_t;
struct devfs_t {
        map_t * tree;
	vnode_t * root;

        fs_t fs;
};

static vnode_t * devfs_get_vnode(vnode_t * dir, const char * name)
{
	devfs_t * fs = container_of(dir->fs, devfs_t, fs);

	return vfstree_get_vnode(fs->tree, dir, name);
}

static vnode_t * devfs_dirnode(devfs_t * devfs)
{
	vnode_t * vnode = calloc(1, sizeof(*vnode));
	vnode_init(vnode, VNODE_DIRECTORY, &devfs->fs);

	return vnode;
}

vnode_t * devfs_open(void)
{
	static devfs_t * devfs = 0;
	static int lock=0;
	SPIN_AUTOLOCK(&lock) {
		static vfs_ops_t ops = {
			get_vnode: devfs_get_vnode,
		};
		if (0==devfs) {
			devfs = calloc(1, sizeof(*devfs));
			devfs->fs.fsops = &ops;
			devfs->tree = treap_new(map_compound_key_comp);

			/* root vnode */
			devfs->root = devfs_dirnode(devfs);

			static char * toplevels[] = {
				"disk",
				"bus",
				"input",
			};
			for(int i=0; i<countof(toplevels); i++) {
				vnode_t * dir = devfs_dirnode(devfs);
				vfstree_put_vnode(devfs->tree, devfs->root, toplevels[i], dir);
			}

			/* console */
			vfstree_put_vnode(devfs->tree, devfs->root, "console", console_dev());
		}
	}

	return devfs->root;
}
