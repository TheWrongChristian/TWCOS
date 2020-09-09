#include "usb.h"

#if INTERFACE

struct urb_t {
	hcd_t * hcd;
	int device;
	int endpoint;
	int direction;
	int flags;
	void * buffer;
	size_t bufferlen;
};

struct usb_device_t {
	uint8_t dev;
	uint8_t endp;
	int ls;
	int toggle;

	/* Periodic information */
	int periodic;
};

#endif

void usb_test(hcd_t * hcd)
{
	static uint8_t config[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	static uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_device_t dev[] = {{0, 0, 1}};

	hcd_packet(hcd, usbsetup, dev, config, countof(config));
	hcd_packet(hcd, usbin, dev, response, countof(response));
	hcd_packet(hcd, usbout, dev, 0, 0);

	static uint8_t setaddress[] = {0x00, 0x5, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0};
	hcd_packet(hcd, usbsetup, dev, setaddress, countof(setaddress));
	hcd_packet(hcd, usbin, dev, 0, 0);
	dev->dev = 1;

	uint8_t * buf = calloc(1, response[0]);
	hcd_packet(hcd, usbsetup, dev, config, countof(config));
	hcd_packet(hcd, usbin, dev, buf, response[0]);
	hcd_packet(hcd, usbout, dev, 0, 0);

}
