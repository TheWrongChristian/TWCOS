#include "hcd.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

struct hcd_ops_t {
	void (*packet)(hcd_t * hcd, usbpid_t pid, uint8_t dev, uint8_t endp, uint8_t ls, void * buf, size_t buflen);
};

struct hcd_t {
	hcd_ops_t * ops;
};

enum usbpid_t { usbsetup, usbin, usbout };

#endif

void hcd_packet(hcd_t * hcd, usbpid_t pid, uint8_t dev, uint8_t endp, uint8_t ls, void * buf, size_t buflen)
{
	hcd->ops->packet( hcd, pid, dev, endp, ls, buf, buflen);
}
