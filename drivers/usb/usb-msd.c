#include "usb-msd.h"

#if INTERFACE


#endif

typedef struct usb_bbb_device_t usb_bbb_device_t;
struct usb_bbb_device_t {
	device_t device;
	block_t block;
	/* cdb_transport_t cdb_transport; */
	usb_endpoint_t * epin;
	usb_endpoint_t * epout;
};

static future_t *  usb_bbb_device_read(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);

	return future_static_success();
}

static future_t *  usb_bbb_device_write(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);

	return future_static_success();
}

static size_t usb_bbb_device_blocksize(block_t * block)
{
        usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);
        return 0;
}

static off64_t usb_bbb_device_getsize(block_t * block)
{
        usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);
        return 0;
}


static interface_map_t usb_bbb_device_t_map [] =
{
        INTERFACE_MAP_ENTRY(usb_bbb_device_t, iid_block_t, block),
        INTERFACE_MAP_ENTRY(usb_bbb_device_t, iid_device_t, device),
};
static INTERFACE_IMPL_QUERY(device_t, usb_bbb_device_t, device)
static INTERFACE_OPS_TYPE(device_t) INTERFACE_IMPL_NAME(device_t, usb_bbb_device_t) = {
        INTERFACE_IMPL_QUERY_METHOD(device_t, usb_bbb_device_t)
        INTERFACE_IMPL_METHOD(probe, 0)
};
static INTERFACE_IMPL_QUERY(block_t, usb_bbb_device_t, block)
static INTERFACE_OPS_TYPE(block_t) INTERFACE_IMPL_NAME(block_t, usb_bbb_device_t) = {
        INTERFACE_IMPL_QUERY_METHOD(block_t, usb_bbb_device_t)
        INTERFACE_IMPL_METHOD(read, usb_bbb_device_read)
        INTERFACE_IMPL_METHOD(write, usb_bbb_device_write)
        INTERFACE_IMPL_METHOD(getsize, usb_bbb_device_getsize)
        INTERFACE_IMPL_METHOD(blocksize, usb_bbb_device_blocksize)
};

static void usb_bbb_device_probe(device_t * device, usb_interface_descriptor_t * interface)
{
	usb_endpoint_t * epin = usb_get_endpoint(device, interface, 1, usbbulk);
	usb_endpoint_t * epout = usb_get_endpoint(device, interface, 0, usbbulk);

	check_not_null(epin, "No BULK IN endpoint found");
	check_not_null(epout, "No BULK OUT endpoint found");

	usb_bbb_device_t * bbb = calloc(1, sizeof(*bbb));
	device_init(&bbb->device, device);
	bbb->device.ops = &usb_bbb_device_t_device_t;
	bbb->block.ops = &usb_bbb_device_t_block_t;
}

void usb_msd_device_probe(device_t * device)
{
	usb_interface_descriptor_t * interface = usb_get_interface_by_class(device, 8, 6, 80);
	if (interface) {
		usb_bbb_device_probe(device, interface);
	}
}
