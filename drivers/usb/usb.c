#include "usb.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

enum usbpid_t { usbsetup, usbin, usbout };

struct hcd_ops_t {
	future_t * (*packet)(usb_endpoint_t * endpoint, usbpid_t pid, void * buf, size_t buflen);
};

struct hcd_t {
	hcd_ops_t * ops;
	/* Enough for 128 ids */
	uint32_t ids[4];
};

struct urb_t {
	hcd_t * hcd;
	usbpid_t pid;
	int device;
	int endpoint;
	int direction;
	int flags;
	void * buffer;
	size_t bufferlen;
};

#define USB_DEVICE_LOW_SPEED 1<<0
#define USB_DEVICE_FULL_SPEED 1<<1
#define USB_DEVICE_HIGH_SPEED 1<<2
#define USB_DEVICE_SUPER_SPEED 1<<3

struct usb_hub_ops_t {
	int (*port_count)(usb_hub_t * hub);
	void (*reset_port)(usb_hub_t * hub, int port);
	usb_device_t * (*get_device)(usb_hub_t * hub, int port);
	void (*disable_port)(usb_hub_t * hub, int port);
	hcd_t * (*get_hcd)(usb_hub_t * hub);
};

struct usb_hub_t {
	usb_hub_ops_t * ops;
	usb_device_t * device;

	int portcount;
	usb_device_t ** ports;
};

struct usb_endpoint_t {
	usb_device_t * device;
	uint8_t endp;
	int toggle;

	/* Periodic information */
	int periodic;
};

struct usb_device_t {
	/* Host controller */
	hcd_t * hcd;

	/* Device address */
	uint8_t dev;
	uint8_t flags;

	/* Topology */
	usb_hub_t * hub;

	/* End points */
	usb_endpoint_t * endpoints;
};

#endif

int usb_hub_port_count(usb_hub_t * hub)
{
	if (!hub->portcount) {
	       hub->portcount=hub->ops->port_count(hub);
	}
	return hub->portcount;
}

void usb_hub_reset_port(usb_hub_t * hub, int port)
{
	check_int_bounds(port, 0, hub->portcount-1, "Invalid port number");
	hub->ops->reset_port(hub, port);
}

usb_device_t * usb_hub_get_device(usb_hub_t * hub, int port)
{
	check_int_bounds(port, 0, hub->portcount-1, "Invalid port number");
	if (!hub->ports[port]) {
		hub->ports[port] = hub->ops->get_device(hub, port);
	}
	return hub->ports[port];
}

static void usb_initialize_device(usb_device_t * device)
{
	uint8_t config[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_endpoint_t endpoint[] = {{.device=device}};

	future_t * f1 = usb_packet(endpoint, usbsetup, config, countof(config));
	future_t * f2 = usb_packet(endpoint, usbin, response, countof(response));
	future_get(f1);
	future_get(f2);

	future_t * f3 = usb_packet(endpoint, usbout, 0, 0);
	future_get(f3);

	/* Allocate an address for the new device */
	int address=0;
	for(int i=1; i<128; i++) {
		if (bitarray_get(device->hcd->ids, i)) {
			bitarray_set(device->hcd->ids, i, 0);
			address = i;
			break;
		}
	}
	if (address) {
		uint8_t setaddress[] = {0x00, 0x5, address, 0x0, 0x0, 0x0, 0x0, 0x0};
		f1 = usb_packet(endpoint, usbsetup, setaddress, countof(setaddress));
		f2 = usb_packet(endpoint, usbin, 0, 0);
		device->dev = address;
		future_get(f1);
		future_get(f2);

		uint8_t * buf = malloc(response[0]);
		config[6] = response[0];
		f1 = usb_packet(endpoint, usbsetup, config, countof(config));
		f2 = usb_packet(endpoint, usbin, buf, response[0]);
		future_get(f1);
		future_get(f2);

		f3 = usb_packet(endpoint, usbout, 0, 0);
		future_get(f3);
	}
}

void usb_hub_enumerate(usb_hub_t * hub)
{
	int portcount = usb_hub_port_count(hub);

	for(int i=0; i<portcount; i++) {
		usb_hub_reset_port(hub, i);
		usb_device_t * device = usb_hub_get_device(hub, i);
		if (device && 0 == device->dev) {
			/* Get the device into an addressed state */
			usb_initialize_device(device);
		}
		hub->ports[i]=device;
		device->hub = hub;
	}
}

hcd_t * usb_hub_get_hcd(usb_hub_t * hub)
{
	while(hub) {
		if (hub->ops->get_hcd) {
			/* This should be the root hub */
			return hub->ops->get_hcd(hub);
		} else {
			/* Walk up the attachment chain */
			hub = hub->device->hub;
		}
	}

	/* Not attached! */
	return 0;
}

future_t * usb_packet(usb_endpoint_t * endpoint, usbpid_t pid, void * buf, size_t buflen)
{
	return endpoint->device->hcd->ops->packet(endpoint, pid, buf, buflen);
}

void usb_test(usb_hub_t * hub)
{
	usb_hub_enumerate(hub);
#if 0
	uint8_t config[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_device_t dev[] = {{0, USB_DEVICE_LOW_SPEED, 0}};
	usb_endpoint_t endpoint[] = {{.device=&dev[0]}};

	TRACE();
	future_t * f1 = usb_packet(usbsetup, endpoint, config, countof(config));
	future_t * f2 = usb_packet(usbin, endpoint, response, countof(response));
	future_get(f1);
	future_get(f2);

	TRACE();
	future_t * f3 = hcd_packet(hcd, usbout, endpoint, 0, 0);
	future_get(f3);

	TRACE();
	static uint8_t setaddress[] = {0x00, 0x5, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
	f1 = hcd_packet(hcd, usbsetup, endpoint, setaddress, countof(setaddress));
	f2 = hcd_packet(hcd, usbin, endpoint, 0, 0);
	dev->dev = 1;

	TRACE();
	future_get(f1);
	future_get(f2);

	TRACE();
	uint8_t * buf = malloc(response[0]);
	config[6] = response[0];
	f1 = hcd_packet(hcd, usbsetup, endpoint, config, countof(config));
	f2 = hcd_packet(hcd, usbin, endpoint, buf, response[0]);
	future_get(f1);
	future_get(f2);

	TRACE();
	f3 = hcd_packet(hcd, usbout, endpoint, 0, 0);
	future_get(f3);
#endif
}
