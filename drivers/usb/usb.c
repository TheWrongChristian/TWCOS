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

struct usb_hub_t {
	uint8_t dev;

	int portcount;
	usb_device_t * ports;
};

struct usb_device_t {
	uint8_t dev;
	uint8_t flags;

	/* Topology */
	usb_hub_t * attachment;
};

struct usb_endpoint_t {
	usb_device_t * device;
	uint8_t endp;
	int toggle;

	/* Periodic information */
	int periodic;
};

#endif

void usb_test(hcd_t * hcd)
{
	uint8_t config[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	static uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_device_t dev[] = {{0, USB_DEVICE_LOW_SPEED, 0}};
	usb_endpoint_t endpoint[] = {{&dev[0]}};

	TRACE();
	future_t * f1 = hcd_packet(hcd, usbsetup, endpoint, config, countof(config));
	future_t * f2 = hcd_packet(hcd, usbin, endpoint, response, countof(response));
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
}
