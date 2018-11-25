#include "dev.h"

#if INTERFACE

struct dev_ops_t {
	void (*submit)( dev_t * dev, buf_op_t * op );
};

struct dev_t {
	dev_ops_t * ops;
};

struct dev_static_t {
	dev_t dev;
	unsigned char * p;
	size_t size;
};

enum dev_op_status { DEV_BUF_OP_SUBMITTED = 0, DEV_BUF_OP_COMPLETE, DEV_BUF_OP_TIMEDOUT, DEV_BUF_OP_FAILED };
struct buf_op_t {
	dev_op_status status;
	int write;
	void * p;
	off_t offset;
	size_t size;
	monitor_t lock[1];
};

#endif

exception_def DeviceException = { "DeviceException", &Exception };
exception_def DeviceTimeoutException = { "DeviceTimeoutException", &DeviceException };

void dev_op_submit( dev_t * dev, buf_op_t * op )
{
	MONITOR_AUTOLOCK(op->lock) {
		op->status = DEV_BUF_OP_SUBMITTED;
		dev->ops->submit(dev, op);
	}
}

dev_op_status dev_op_wait( buf_op_t * op )
{
	MONITOR_AUTOLOCK(op->lock) {
		while(op->status == DEV_BUF_OP_SUBMITTED) {
			monitor_wait(op->lock);
		}
	}

	return op->status;
}

static void dev_static_submit(dev_t * dev, buf_op_t * op )
{
	dev_static_t * sdev = container_of(dev, dev_static_t, dev);

	if (op->write) {
		memcpy(sdev->p+op->offset, op->p, op->size);
	} else {
		memcpy(op->p, sdev->p+op->offset, op->size);
	}
	op->status = DEV_BUF_OP_COMPLETE;
}

dev_t * dev_static(void * p, size_t size)
{
	static dev_ops_t ops = { submit: dev_static_submit };
	dev_static_t * sdev = malloc(sizeof(*sdev));
	sdev->dev.ops = &ops;
	sdev->p = p;
	sdev->size = size;

	return &sdev->dev;
}

typedef struct {
	vnode_t vnode;
	dev_t * dev;
} dev_vnode_t;

size_t dev_read(vnode_t * vnode, off_t offset, void * buf, size_t len)
{
	dev_vnode_t * devnode = container_of(vnode, dev_vnode_t, vnode);
	buf_op_t op = { write: 0, p: buf, offset: offset, size: len };

	dev_op_submit(devnode->dev, &op);
	dev_op_status status = dev_op_wait(&op);

	return op.size;
}

size_t dev_write(vnode_t * vnode, off_t offset, void * buf, size_t len)
{
	dev_vnode_t * devnode = container_of(vnode, dev_vnode_t, vnode);
	buf_op_t op = { write: 1, p: buf, offset: offset, size: len };

	dev_op_submit(devnode->dev, &op);
	dev_op_status status = dev_op_wait(&op);

	return op.size;
}

vnode_t * dev_vnode(dev_t * dev)
{
	static vfs_ops_t ops = { read: dev_read, write: dev_write };
	static fs_t devfs = { fsops: &ops };
	dev_vnode_t * vnode = calloc(1, sizeof(*vnode));
	
	vnode_init(&vnode->vnode, VNODE_DEV, &devfs);
	vnode->dev = dev;

	return &vnode->vnode;
}
