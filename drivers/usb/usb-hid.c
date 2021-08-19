#include "usb-hid.h"

#if INTERFACE


#endif

static void usb_hid_interrupt(usb_endpoint_t * endpoint, void * arg, void * buffer, int bufferlen)
{
	uint8_t * p = buffer;

	kernel_debug("Report: %hhx %hhd %hhd %hhd\n", p[0], p[1], p[2], p[3]);
}

void usb_hid_device_probe(device_t * device)
{
	usb_interface_descriptor_t * interface = usb_get_interface_by_class(device, 3, 1, 0);
	usb_endpoint_t * ep = usb_get_endpoint(device, interface, 1, usbinterrupt);
#if 0
        KTRY {
                uint8_t * hid = usb_get_descriptor(usbdevice, usbdevice->configuration->hid->descriptortype, 0, usbdevice->configuration->hid->descriptorlength);
        } KCATCH(UsbNakException) {
        }
#endif
        usb_set_idle(device);
        usb_interrupt(ep, 0, usb_hid_interrupt);
}
