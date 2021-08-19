#include "block.h"

#if INTERFACE

#include <sys/types.h>

struct block_t_ops {
	void * (*query)(void *, iid_t i);
	future_t * (*read)(block_t * block, void * buf, size_t buflen, off64_t offset);
	future_t * (*write)(block_t * block, void * buf, size_t buflen, off64_t offset);
	off64_t  (*getsize)(block_t * block);
	size_t  (*blocksize)(block_t * block);
};

struct block_t {
	block_t_ops * ops;
};

struct block_static_t {
	device_t device;
	block_t block;
	unsigned char * p;
	off64_t size;
	size_t blocksize;
};

#endif

exception_def BlockException = {"BlockException", &Exception};
exception_def BlockAlignmentException = {"BlockAlignmentException", &BlockException};

static future_t *  block_static_read(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	block_static_t * sblock = container_of(block, block_static_t, block);

	memcpy(buf, sblock->p + offset, buflen);

	return future_static_success();
}

static future_t *  block_static_write(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	block_static_t * sblock = container_of(block, block_static_t, block);

	memcpy(sblock->p + offset, buf, buflen);

	return future_static_success();
}

static size_t block_static_blocksize(block_t * block)
{
	block_static_t * sblock = container_of(block, block_static_t, block);
	return sblock->blocksize;
}

static off64_t block_static_getsize(block_t * block)
{
	block_static_t * sblock = container_of(block, block_static_t, block);
	return sblock->size;
}

static interface_map_t block_static_t_map [] =
{
	INTERFACE_MAP_ENTRY(block_static_t, iid_block_t, block),
	INTERFACE_MAP_ENTRY(block_static_t, iid_device_t, device),
};
static INTERFACE_IMPL_QUERY(device_t, block_static_t, device)
static INTERFACE_OPS_TYPE(device_t) INTERFACE_IMPL_NAME(device_t, block_static_t) = {
	INTERFACE_IMPL_QUERY_METHOD(device_t, block_static_t)
	INTERFACE_IMPL_METHOD(probe, 0)
};
static INTERFACE_IMPL_QUERY(block_t, block_static_t, block)
static INTERFACE_OPS_TYPE(block_t) INTERFACE_IMPL_NAME(block_t, block_static_t) = {
	INTERFACE_IMPL_QUERY_METHOD(block_t, block_static_t)
	INTERFACE_IMPL_METHOD(read, block_static_read)
	INTERFACE_IMPL_METHOD(write, block_static_write)
	INTERFACE_IMPL_METHOD(getsize, block_static_getsize)
	INTERFACE_IMPL_METHOD(blocksize, block_static_blocksize)
};

block_t * block_static(void * p, size_t size)
{
	block_static_t * sblock = malloc(sizeof(*sblock));
	sblock->block.ops = &block_static_t_block_t;
	sblock->p = p;
	sblock->size = size;
	sblock->blocksize = 512;

	return &sblock->block;
}

static void block_check(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	off64_t size = block_getsize(block);
	size_t blocksize = block_blocksize(block);

	if (buflen % blocksize) {
		KTHROWF(BlockAlignmentException, "Buffer length not block aligned with %zx: %zx", blocksize, buflen);
	}
	if (offset % blocksize) {
		KTHROWF(BlockAlignmentException, "Buffer offset not block aligned with %zx: %llx", blocksize, offset);
	}
	if (offset + buflen > size) {
		KTHROWF(IntBoundsException, "Offset + size exceeds block device size %llx: %llx", size, offset + buflen);
	}
}

future_t * block_read(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	block_check(block, buf, buflen, offset);
	return block->ops->read(block, buf, buflen, offset);
}

future_t * block_write(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	block_check(block, buf, buflen, offset);
	return block->ops->write(block, buf, buflen, offset);
}

off64_t  block_getsize(block_t * block)
{
	return block->ops->getsize(block);
}

size_t  block_blocksize(block_t * block)
{
	return block->ops->blocksize(block);
}

char iid_block_t[]="Block device";

#if 0
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
	dev_t * dev;
	dev_op_status status;
	exception_cause * cause;
	int write;
	void * p;
	off64_t offset;
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
		op->dev = dev;
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

	if (op->status != DEV_BUF_OP_COMPLETE && op->cause) {
		exception_throw_cause(op->cause);
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

size_t dev_read(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	dev_vnode_t * devnode = container_of(vnode, dev_vnode_t, vnode);
	buf_op_t op = { write: 0, p: buf, offset: offset, size: len };

	dev_op_submit(devnode->dev, &op);
	dev_op_wait(&op);

	return op.size;
}

size_t dev_write(vnode_t * vnode, off64_t offset, void * buf, size_t len)
{
	dev_vnode_t * devnode = container_of(vnode, dev_vnode_t, vnode);
	buf_op_t op = { write: 1, p: buf, offset: offset, size: len };

	dev_op_submit(devnode->dev, &op);
	dev_op_wait(&op);

	return op.size;
}

static vmpage_t * dev_get_page(vnode_t * vnode, off64_t offset)
{
	arena_state state = arena_getstate(NULL);
	void * p = arena_palloc(NULL, 1);
	dev_read(vnode, ROUNDDOWN(offset, ARCH_PAGE_SIZE), p, ARCH_PAGE_SIZE);
	vmpage_t * vmpage = vm_page_steal(p);

	arena_setstate(NULL, state);

	return vmpage;
}

#if 0
static void dev_put_page(vnode_t * vnode, off64_t offset, vmpage_t * page)
{
}
#endif

vnode_t * dev_vnode(dev_t * dev)
{
	static vnode_ops_t ops = { get_page: dev_get_page, read: dev_read, write: dev_write };
	static fs_t devfs = { vnodeops: &ops };
	dev_vnode_t * vnode = calloc(1, sizeof(*vnode));
	
	vnode->dev = dev;
	return vnode_init(&vnode->vnode, VNODE_DEV, &devfs);
}
#endif
