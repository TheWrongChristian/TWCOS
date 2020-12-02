#include "usb.h"

#if INTERFACE

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
	usb_device_t device;

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
	uint8_t dev;
	uint8_t flags;

	/* Topology */
	usb_hub_t * attachment;

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
	check_int_bounds(port, 1, hub->portcount, "Invalid port number");
	hub->ops->reset_port(hub, port);
}

usb_device_t * usb_hub_get_device(usb_hub_t * hub, int port)
{
	check_int_bounds(port, 1, hub->portcount, "Invalid port number");
	if (!hub->ports[port-1]) {
		hub->ports[port-1] = hub->ops->get_device(hub, port);
	}
	return hub->ports[port-1];
}

void usb_hub_enumerate(usb_hub_t * hub)
{
	int portcount = usb_hub_port_count(hub);

	for(int i=1; i<=portcount; i++) {
		usb_hub_reset_port(hub, i);
		usb_device_t * device = usb_hub_get_device(hub, i);
		if (device) {
		}
		hub->ports[i-1]=device;
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
			hub = hub->device.attachment;
		}
	}

	/* Not attached! */
	return 0;
}

void usb_test(hcd_t * hcd)
{
	uint8_t config[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	static uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_device_t dev[] = {{0, USB_DEVICE_LOW_SPEED, 0}};
	usb_endpoint_t endpoint[] = {{&dev[0]}};

	future_t * f1 = hcd_packet(hcd, usbsetup, endpoint, config, countof(config));
	future_t * f2 = hcd_packet(hcd, usbin, endpoint, response, countof(response));
	future_get(f1);
	future_get(f2);

	future_t * f3 = hcd_packet(hcd, usbout, endpoint, 0, 0);
	future_get(f3);

	static uint8_t setaddress[] = {0x00, 0x5, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
	f1 = hcd_packet(hcd, usbsetup, endpoint, setaddress, countof(setaddress));
	f2 = hcd_packet(hcd, usbin, endpoint, 0, 0);
	dev->dev = 1;

	future_get(f1);
	future_get(f2);

	uint8_t * buf = malloc(response[0]);
	config[6] = response[0];
	f1 = hcd_packet(hcd, usbsetup, endpoint, config, countof(config));
	f2 = hcd_packet(hcd, usbin, endpoint, buf, response[0]);
	future_get(f1);
	future_get(f2);

	f3 = hcd_packet(hcd, usbout, endpoint, 0, 0);
	future_get(f3);
}
