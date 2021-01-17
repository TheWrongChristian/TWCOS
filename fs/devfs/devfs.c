#include "devfs.h"

vnode_t * devfs_open(void)
{
	static vnode_t * root = 0;
	static int lock=0;
	SPIN_AUTOLOCK(&lock) {
		if (0 == root) {
			root = vfstree_new();
			static char * toplevels[] = {
				"disk",
				"bus",
				"input",
			};
			for(int i=0; i<countof(toplevels); i++) {
				vnode_newdir(root, toplevels[i]);
			}

			/* console */
			vnode_put_vnode(root, "console", console_dev());
		}
	}

	return root;
}
