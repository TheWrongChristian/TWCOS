#include "dev.h"

#if INTERFACE

typedef struct dev_ops_s {
	void (*submit)( dev_s * dev, buf_op_s * op );
} dev_ops_t;

typedef struct dev_s {
	dev_ops_t * ops;
} dev_t;

typedef struct dev_static_s {
	dev_ops_t * ops;
	char * p;
	size_t size;
} dev_static_t;

enum dev_op_status { DEV_BUF_OP_SUBMITTED = 0, DEV_BUF_OP_COMPLETE, DEV_BUF_OP_TIMEDOUT, DEV_BUF_OP_FAILED };
typedef struct buf_op_s {
	dev_op_status status;
	int write;
	void * p;
	off_t offset;
	size_t size;
} buf_op_t;

#endif

exception_def DeviceException = { "DeviceException", &Exception };
exception_def DeviceTimeoutException = { "DeviceTimeoutException", &DeviceException };

void dev_op_submit( dev_t * dev, buf_op_t * op )
{
	thread_lock(op);
	op->status = DEV_BUF_OP_SUBMITTED;
	dev->ops->submit(dev, op);
	thread_unlock(op);
}

dev_op_status dev_op_wait( buf_op_t * op )
{
	thread_lock(op);
	while(op->status == DEV_BUF_OP_SUBMITTED) {
		thread_wait(op);
	}
	thread_unlock(op);

	return op->status;
}

static void dev_static_submit(dev_t * dev, buf_op_t * op )
{
	dev_static_t * sdev = (dev_static_t*)dev;

	if (op->write) {
		memcpy(sdev->p+op->offset, op->p, op->size);
	} else {
		memcpy(op->p, sdev->p+op->offset, op->size);
	}
	op->status = DEV_BUF_OP_COMPLETE;
}

dev_t * dev_static(char * p, size_t size)
{
	static dev_ops_t ops = { submit: dev_static_submit };
	dev_static_t * sdev = malloc(sizeof(*sdev));
	sdev->ops = &ops;
	sdev->p = p;
	sdev->size = size;

	return sdev;
}
