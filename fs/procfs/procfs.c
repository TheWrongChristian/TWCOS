#include "procfs.h"

#if INTERFACE

#endif

vnode_t * procfs_open(dev_t * dev)
{
        vnode_t * root = vfstree_new();

	return root;
}
