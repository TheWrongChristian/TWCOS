#include "usb.h"

#if INTERFACE

struct usb_device_t {
};

struct urb_t {
	hcd_t * hcd;
	int device;
	int endpoint;
	int direction;
	int flags;
	void * buffer;
	size_t bufferlen;
};

#endif
