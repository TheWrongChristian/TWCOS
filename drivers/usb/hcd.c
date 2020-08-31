#include "hcd.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

struct hcd_ops_t {
	void (*packet)(hcd_t * hcd, usbpid_t pid, usb_device_t * dev, void * buf, size_t buflen);
};

struct hcd_t {
	hcd_ops_t * ops;
};

enum usbpid_t { usbsetup, usbin, usbout };

#endif

void hcd_packet(hcd_t * hcd, usbpid_t pid, usb_device_t * dev, void * buf, size_t buflen)
{
	hcd->ops->packet( hcd, pid, dev, buf, buflen);
}
