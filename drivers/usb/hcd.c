#include "hcd.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

struct hcd_ops_t {
	future_t * (*packet)(hcd_t * hcd, usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen);
};

struct hcd_t {
	hcd_ops_t * ops;
};

enum usbpid_t { usbsetup, usbin, usbout };

#endif

future_t * hcd_packet(hcd_t * hcd, usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen)
{
	return hcd->ops->packet( hcd, pid, endpoint, buf, buflen);
}
