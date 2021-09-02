#include "usb-msd.h"

#if INTERFACE


#endif

typedef struct usb_bbb_device_t usb_bbb_device_t;
struct usb_bbb_device_t {
	device_t device;
	block_t block;
	cam_capacity_t capacity;
	/* cdb_transport_t cdb_transport; */
	struct {
		usb_request_t request[1];
		uint8_t buf[31];
	} cbw;
	struct {
		usb_request_t request[1];
		uint8_t * buf;
	} transfer;
	struct {
		usb_request_t request[1];
		uint8_t buf[13];
	} csw;
	usb_endpoint_t * epin;
	usb_endpoint_t * epout;
	future_t * status_future;
	future_t future[1];
};

static packet_field_t cbwfields[] = {
	/* dCBWSignature - 0x43425355 */
	PACKET_LEFIELD(4),
	/* dCBWTag */
	PACKET_LEFIELD(4),
	/* dCBWDataTransferLength */
	PACKET_LEFIELD(4),
	/* dCBWFlags */
	PACKET_LEFIELD(1),
	/* dCBWLun */
	PACKET_LEFIELD(1),
	/* dCBWLength */
	PACKET_LEFIELD(1),
	/* Wrapped command */
	PACKET_FIELD(16)
};
static packet_def_t cbw[]={PACKET_DEF(cbwfields)}; 

static packet_field_t cswfields[] = {
	/* dCSWSignature - 0x53425355 */
	PACKET_LEFIELD(4),
	/* dCSWTag */
	PACKET_LEFIELD(4),
	/* dCSWDataResidue */
	PACKET_LEFIELD(4),
	/* dCSWStatus */
	PACKET_LEFIELD(1),
};
static packet_def_t csw[]={PACKET_DEF(cswfields)}; 

exception_def UMSException = {"UMSException", &BlockException};

static future_t * usb_bbb_command(usb_bbb_device_t * bbb, uint8_t * cmd, int cmdlen, uint32_t transferlength, uint8_t flags, uint8_t lun)
{
	packet_set(cbw, bbb->cbw.buf, 0, 0x43425355);
	packet_set(cbw, bbb->cbw.buf, 1, (uint32_t)bbb);
	packet_set(cbw, bbb->cbw.buf, 2, transferlength);
	packet_set(cbw, bbb->cbw.buf, 3, flags);
	packet_set(cbw, bbb->cbw.buf, 4, lun);
	packet_set(cbw, bbb->cbw.buf, 5, cmdlen);
	void * const wrapped = packet_subpacket(cbw, bbb->cbw.buf, 6, 16);
	memcpy(wrapped, cmd, cmdlen);

	/* bbb->cbw.buf is now the CBW we want to transfer */
	return usb_submit(bbb->cbw.request);
}

static future_t * usb_bbb_status(usb_bbb_device_t * bbb)
{
	return usb_submit(bbb->csw.request);
}

exception_def UMSCSWException = {"UMSCSWException", &UMSException};
static uint8_t usb_bbb_get_status(usb_bbb_device_t * bbb)
{
	if (0x53425355 != packet_get(csw, bbb->csw.buf, 0)) {
		KTHROWF(UMSCSWException, "CSW wrapper signature invalid: Got 0x%x", packet_get(csw, bbb->csw.buf, 0));
	}
	if ((uint32_t)bbb != packet_get(csw, bbb->csw.buf, 1)) {
		KTHROWF(UMSCSWException, "CSW wrapper signature invalid: Expected 0x%x, got 0x%x", (uint32_t)bbb, packet_get(csw, bbb->csw.buf, 1));
	}

	return (uint8_t)packet_get(csw, bbb->csw.buf, 3);
}

static future_t * usb_bbb_transfer(usb_bbb_device_t * bbb, void * buf, size_t buflen, int in)
{
	if (in) {
		usb_bulk_request(bbb->epin, buf, buflen, bbb->transfer.request);
	} else {
		usb_bulk_request(bbb->epout, buf, buflen, bbb->transfer.request);
	}
	return usb_submit(bbb->transfer.request);
}

static void usb_bbb_transfer_complete(usb_bbb_device_t * bbb)
{
	KTRY {
		future_get_timeout(bbb->status_future, 5000000);
		future_set(bbb->future, usb_bbb_get_status(bbb));
	} KCATCH(UsbStallException) {
		/* Clear stall */
		usb_set_endpoint_halt(bbb->epin, 0);
		usb_set_endpoint_halt(bbb->epout, 0);
	} KCATCH(Throwable) {
		future_cancel(bbb->future, exception_get_cause());
	}
}

static void usb_bbb_device_async(void * p)
{
	usb_bbb_device_t * bbb = p;

	usb_bbb_transfer_complete(bbb);
}

static future_t * usb_bbb_device_request(usb_bbb_device_t * bbb, uint8_t * cmd, size_t cmdlen, void * buf, size_t buflen, int transferin)
{
	future_get(bbb->future);
#if 0
	future_init(bbb->future, 0, 0);
	future_t * command_future = usb_bbb_command(bbb, cmd, cmdlen, buflen, 0x80, 0);
	if (buf && buflen) {
		future_t * transfer_future = usb_bbb_transfer(bbb, buf, buflen, transferin);
		bbb->status_future = usb_bbb_status(bbb);
		future_chain(transfer_future, command_future);
		future_chain(bbb->status_future, transfer_future);
	} else {
		bbb->status_future = usb_bbb_status(bbb);
		future_chain(bbb->status_future, command_future);
	}

	thread_pool_submit(0, usb_bbb_device_async, bbb);
#else
	future_init(bbb->future, 0, 0);
	future_t * command_future = usb_bbb_command(bbb, cmd, cmdlen, buflen, 0x80, 0);
	future_get(command_future);
	if (buf && buflen) {
		future_t * transfer_future = usb_bbb_transfer(bbb, buf, buflen, transferin);
		future_get(transfer_future);
		bbb->status_future = usb_bbb_status(bbb);
	} else {
		bbb->status_future = usb_bbb_status(bbb);
	}
	usb_bbb_transfer_complete(bbb);
#endif

	return bbb->future;
}

static future_t * usb_bbb_device_read(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);

	future_get(bbb->future);

	uint8_t readcmd[10];
	cam_cmd_read10(readcmd, countof(readcmd), offset >> 9, buflen / bbb->capacity.blocksize);
	return usb_bbb_device_request(bbb, readcmd, countof(readcmd), buf, buflen, 1);
}

static future_t * usb_bbb_device_write(block_t * block, void * buf, size_t buflen, off64_t offset)
{
	usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);

	future_get(bbb->future);

	uint8_t writecmd[10];
	cam_cmd_read10(writecmd, countof(writecmd), offset >> 9, buflen / bbb->capacity.blocksize);
	return usb_bbb_device_request(bbb, writecmd, countof(writecmd), buf, buflen, 0);
}

static void usb_bbb_device_inquiry(usb_bbb_device_t * bbb)
{
	uint8_t cmd[6];
	uint8_t inquirydata[128];
	cam_cmd_inquiry(cmd, countof(cmd), countof(inquirydata));
	future_t * future = usb_bbb_device_request(bbb, cmd, countof(cmd), inquirydata, countof(inquirydata), 1);
	future_get(future);
}

static void usb_bbb_read_capacity(usb_bbb_device_t * bbb)
{
	uint8_t cmd[10];
	uint8_t response[8] = {0};
	cam_cmd_read_capacity10(cmd, countof(cmd));
	future_t * future = usb_bbb_device_request(bbb, cmd, countof(cmd), response, countof(response), 1);
	future_get(future);
	cam_response_read_capacity(response, countof(response), &bbb->capacity);
}

static void usb_bbb_test_unit_ready(usb_bbb_device_t * bbb)
{
	uint8_t cmd[6];
	cam_cmd_test_unit_ready(cmd, countof(cmd));
	future_t * future = usb_bbb_device_request(bbb, cmd, countof(cmd), 0, 0, 0);
	future_get(future);
}

static size_t usb_bbb_device_blocksize(block_t * block)
{
        usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);
        return bbb->capacity.blocksize;
}

static off64_t usb_bbb_device_getsize(block_t * block)
{
        usb_bbb_device_t * bbb = container_of(block, usb_bbb_device_t, block);
        return bbb->capacity.blocks * bbb->capacity.blocksize;
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

static void usb_bbb_test(device_t * device)
{
	block_t * block = device->ops->query(device, iid_block_t);

	static char buf[512];
	future_t * future = block_read(block, buf, countof(buf), 0);
	future_get_timeout(future, 5000000);
}

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
	bbb->epin = epin;
	bbb->epout = epout;
	usb_bulk_request(bbb->epout, bbb->cbw.buf, countof(bbb->cbw.buf), bbb->cbw.request);
	usb_bulk_request(bbb->epin, bbb->csw.buf, countof(bbb->csw.buf), bbb->csw.request);
#if 1
	usb_bbb_device_inquiry(bbb);
#endif
	usb_bbb_read_capacity(bbb);
	usb_bbb_test_unit_ready(bbb);
	device_queue(&bbb->device, "block", 0);
	usb_bbb_test(&bbb->device);
}

void usb_msd_device_probe(device_t * device)
{
	usb_interface_descriptor_t * interface = usb_get_interface_by_class(device, 8, 6, 80);
	if (interface) {
		usb_bbb_device_probe(device, interface);
	}
}
