#include "usb.h"

#if INTERFACE

#include <stdint.h>
#include <sys/types.h>

enum usbpid_t { usbsetup, usbin, usbout };

struct usb_request_t {
	usb_endpoint_t * endpoint;

	/* Data buffer if required */
	uint8_t * buffer;
	int bufferlen;

	/* Control requests */	
	uint8_t * control;
	int controllen;

}

struct hcd_t_ops {
	void * (*query)(void * hcd, iid_t iid);
	future_t * (*submit)(hcd_t * hcd, usb_request_t * request);
};

struct hcd_t {
	hcd_t_ops * ops;
	/* Enough for 128 ids */
	uint32_t ids[4];
};

#define USB_DEVICE_LOW_SPEED 1<<0
#define USB_DEVICE_FULL_SPEED 1<<1
#define USB_DEVICE_HIGH_SPEED 1<<2
#define USB_DEVICE_SUPER_SPEED 1<<3

struct usb_hub_t_ops {
	void * (*query)(void * hub, iid_t iid);
	int (*port_count)(usb_hub_t * hub);
	void (*reset_port)(usb_hub_t * hub, int port);
	usb_device_t * (*get_device)(usb_hub_t * hub, int port);
	void (*disable_port)(usb_hub_t * hub, int port);
	hcd_t * (*get_hcd)(usb_hub_t * hub);
};

struct usb_hub_t {
	usb_hub_t_ops * ops;
	usb_device_t * device;

	int portcount;
	usb_device_t ** ports;
};

struct usb_port_t {
};

struct usb_endpoint_t {
	usb_device_t * device;
	int toggle;

	/* Endpoint information */
	usb_endpoint_descriptor_t * descriptor;

	/* Periodic information */
	int periodic;
};

struct usb_device_t {
	/* Device manager generic interface */
	device_t * device;

	/* Host controller to which this device is attached */
	hcd_t * hcd;

	/* Default control endpoint */
	usb_endpoint_t controlep[1];

	/* Device address */
	uint8_t dev;
	uint8_t flags;

	/* Topology */
	usb_hub_t * hub;

	/* Configuration */
	usb_configuration_descriptor_t * configuration;
};

struct usb_device_descriptor_t {
	int class;
	int subclass;
	int protocol;
	int maxpacketsize;
	int vendorid;
	int productid;
	char * manufacturer;
	char * product;
	char * serialnumber;
	int numconfigurations;
	usb_configuration_descriptor_t ** configurations;
};
struct usb_configuration_descriptor_t {
	usb_device_descriptor_t * device;
	int totallength;
	int configuration;
	int numinterfaces;
	usb_interface_descriptor_t ** interfaces;
};
struct usb_interface_descriptor_t {
	usb_configuration_descriptor_t * configuration;
	int number;
	int class;
	int subclass;
	int protocol;
	int numendpoints;
	usb_endpoint_descriptor_t ** endpoints;
};
struct usb_endpoint_descriptor_t {
	usb_interface_descriptor_t * interface;
	uint8_t endpoint;
	uint8_t attributes;
	int maxpacketsize;
	int interval;
};

#endif



static packet_field_t usbdescriptorfields[] = {
	/** bLength */
	PACKET_LEFIELD(1),
	/** bDescriptorType */
	PACKET_LEFIELD(1),
};
static packet_def_t usbdescriptor[]={PACKET_DEF(usbdescriptorfields)};

static uint32_t usb_descriptor_len(uint8_t * packet, int packetlen)
{
	return packet_get(usbdescriptor, packet, 0);
}

static uint32_t usb_descriptor_type(uint8_t * packet, int packetlen)
{
	return packet_get(usbdescriptor, packet, 1);
}

#if 0
static packet_field_t usbsetupfields[] = {
	PACKET_LEFIELD(1),
	PACKET_LEFIELD(1),
	PACKET_LEFIELD(2),
	PACKET_LEFIELD(2),
	PACKET_LEFIELD(2),
};
static packet_def_t usbsetup[]={PACKET_DEF(usbsetupfields)};
#endif

usb_device_descriptor_t * usb_parse_device_descriptor(uint8_t * packet, int packetlen)
{
	static packet_field_t usbdevicedescriptorfields[] = {
		/** bLength */
		PACKET_LEFIELD(1),
		/** bDescriptorType */
		PACKET_LEFIELD(1),
		/** bcdUSB */
		PACKET_LEFIELD(2),
		/** bDeviceClass */
		PACKET_LEFIELD(1),
		/** bDeviceSubClass */
		PACKET_LEFIELD(1),
		/** bDeviceProtocol */
		PACKET_LEFIELD(1),
		/** bMaxPacketSize */
		PACKET_LEFIELD(1),
		/** idVendor */
		PACKET_LEFIELD(2),
		/** idProduct */
		PACKET_LEFIELD(2),
		/** bcdDevice */
		PACKET_LEFIELD(2),
		/** iManufacturer */
		PACKET_LEFIELD(1),
		/** iProduct */
		PACKET_LEFIELD(1),
		/** iSerialNumber */
		PACKET_LEFIELD(1),
		/** bNumConfigurations */
		PACKET_LEFIELD(1),
	};
	static packet_def_t usbdevicedescriptor[]={PACKET_DEF(usbdevicedescriptorfields)};
	usb_device_descriptor_t * descriptor = calloc(1, sizeof(*descriptor));

	descriptor->class = packet_get(usbdevicedescriptor, packet, 3);
	descriptor->subclass = packet_get(usbdevicedescriptor, packet, 4);
	descriptor->protocol = packet_get(usbdevicedescriptor, packet, 5);
	descriptor->maxpacketsize = packet_get(usbdevicedescriptor, packet, 6);
	descriptor->numconfigurations = packet_get(usbdevicedescriptor, packet, -1);
	descriptor->configurations = calloc(descriptor->numconfigurations, sizeof(*descriptor->configurations));

	return descriptor;
}

static usb_configuration_descriptor_t * usb_parse_configuration_descriptor(usb_device_descriptor_t * device, uint8_t * packet, int packetlen)
{
	static packet_field_t usbconfigurationdescriptorfields[] = {
		/** bLength */
		PACKET_LEFIELD(1),
		/** bDescriptorType */
		PACKET_LEFIELD(1),
		/** wTotalLength */
		PACKET_LEFIELD(2),
		/** bNumInterfaces */
		PACKET_LEFIELD(1),
		/** bConfigurationValue */
		PACKET_LEFIELD(1),
		/** iConfiguration */
		PACKET_LEFIELD(1),
		/** bmAttributes */
		PACKET_LEFIELD(1),
		/** bMaxPower */
		PACKET_LEFIELD(1),
	};
	static packet_def_t usbconfigurationdescriptor[]={PACKET_DEF(usbconfigurationdescriptorfields)};
	usb_configuration_descriptor_t * configuration = calloc(1, sizeof(*configuration));
	configuration->device = device;
	configuration->configuration = packet_get(usbconfigurationdescriptor, packet, 5);
	configuration->totallength = packet_get(usbconfigurationdescriptor, packet, 2);
	configuration->numinterfaces = packet_get(usbconfigurationdescriptor, packet, 3);
	configuration->interfaces = calloc(configuration->numinterfaces, sizeof(*configuration->interfaces));

	return configuration;
}

static usb_interface_descriptor_t * usb_parse_interface_descriptor(usb_configuration_descriptor_t * configuration, uint8_t * packet, int packetlen)
{
	static packet_field_t usbinterfacedescriptorfields[] = {
		/** bLength */
		PACKET_LEFIELD(1),
		/** bDescriptorType */
		PACKET_LEFIELD(1),
		/** bInterfaceNumber */
		PACKET_LEFIELD(1),
		/** bAlternateSetting */
		PACKET_LEFIELD(1),
		/** bNumEndpoints */
		PACKET_LEFIELD(1),
		/** bInterfaceClass */
		PACKET_LEFIELD(1),
		/** bInterfaceSubClass */
		PACKET_LEFIELD(1),
		/** bInterfaceProtocol */
		PACKET_LEFIELD(1),
		/** iInterface */
		PACKET_LEFIELD(1),
	};
	static packet_def_t usbinterfacedescriptor[]={PACKET_DEF(usbinterfacedescriptorfields)};
	usb_interface_descriptor_t * interface = calloc(1, sizeof(*interface));
	interface->configuration = configuration;
	interface->number = packet_get(usbinterfacedescriptor, packet, 2);
	interface->numendpoints = packet_get(usbinterfacedescriptor, packet, 4);
	interface->class = packet_get(usbinterfacedescriptor, packet, 5);
	interface->subclass = packet_get(usbinterfacedescriptor, packet, 6);
	interface->protocol = packet_get(usbinterfacedescriptor, packet, 7);
	interface->endpoints = calloc(interface->numendpoints, sizeof(*interface->endpoints));

	return interface;
}

static usb_endpoint_descriptor_t * usb_parse_endpoint_descriptor(usb_interface_descriptor_t * interface, uint8_t * packet, int packetlen)
{
	static packet_field_t usbendpointdescriptorfields[] = {
		/** bLength */
		PACKET_LEFIELD(1),
		/** bDescriptorType */
		PACKET_LEFIELD(1),
		/** bEndpointAddress */
		PACKET_LEFIELD(1),
		/** bmAttributes */
		PACKET_LEFIELD(1),
		/** wMaxPacketSize */
		PACKET_LEFIELD(2),
		/** bInterval */
		PACKET_LEFIELD(1),
	};
	static packet_def_t usbendpointdescriptor[]={PACKET_DEF(usbendpointdescriptorfields)};
	usb_endpoint_descriptor_t * endpoint = calloc(1, sizeof(*endpoint));
	endpoint->interface = interface;
	endpoint->endpoint = packet_get(usbendpointdescriptor, packet, 2);
	endpoint->attributes = packet_get(usbendpointdescriptor, packet, 3);
	endpoint->maxpacketsize = packet_get(usbendpointdescriptor, packet, 4);
	endpoint->interval = packet_get(usbendpointdescriptor, packet, 5);

	return endpoint;
}

usb_configuration_descriptor_t * usb_parse_configuration_descriptor_packet(usb_device_descriptor_t * device, uint8_t * packet, int packetlen)
{
	usb_configuration_descriptor_t * configuration = 0;
	int nextinterface = -1;
	usb_interface_descriptor_t * interface = 0;
	int nextendpoint = -1;
	usb_endpoint_descriptor_t * endpoint = 0;

	uint8_t * next = packet;
	int remaining = packetlen;

	while(remaining>0) {
		const int descriptorlen = usb_descriptor_len(next, remaining);

		switch(usb_descriptor_type(next, remaining))
		{
		case 2:
			configuration = usb_parse_configuration_descriptor(device, next, remaining);
			nextinterface = 0;
			break;
		case 4:
			check_not_null(configuration, "");
			check_int_bounds(nextinterface, 0, configuration->numinterfaces-1, "");
			interface = usb_parse_interface_descriptor(configuration, next, remaining);
			configuration->interfaces[nextinterface++] = interface;
			nextendpoint = 0;
			break;
		case 5:
			check_not_null(interface, "");
			check_int_bounds(nextendpoint, 0, interface->numendpoints-1, "");
			endpoint = usb_parse_endpoint_descriptor(interface, next, remaining);
			interface->endpoints[nextendpoint++] = endpoint;
			break;
		}
		next += descriptorlen;
		remaining -= descriptorlen;
	}

	return configuration;
}

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

static device_type_t usb_type()
{
	static device_type_t type = 0;
	if (0 == type) {
		type = device_type("bus/usb");
	}

	return type;
}

static void usb_bulk_request(usb_endpoint_t * endpoint, void * buffer, size_t bufferlen, usb_request_t * request)
{
	request->endpoint = endpoint;
	request->buffer = buffer;
	request->bufferlen = bufferlen;
	request->control = 0;
}

static void usb_interrupt_request(usb_endpoint_t * endpoint, void * buffer, size_t bufferlen, usb_request_t * request)
{
	request->endpoint = endpoint;
	request->buffer = buffer;
	request->bufferlen = bufferlen;
	request->control = 0;
}

static void usb_control_request(usb_device_t * device, uint8_t * control, int controllen, uint8_t * buffer, int bufferlen, usb_request_t * request)
{
	request->endpoint = device->controlep;
	request->control = control;
	request->controllen = controllen;
	request->buffer = buffer;
	request->bufferlen = bufferlen;
}

static usb_device_t * usb_device(device_t * device)
{
	return container_of(device, usb_device_t, device);
}

static void usb_initialize_device(usb_device_t * device)
{
	/* Setup the control endpoint */
	device->controlep->device = device;
	uint8_t getdevice[] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x8, 0x0};
	uint8_t response[] = {0, 0, 0, 0, 0, 0, 0, 0};
	usb_request_t request[1];
	usb_control_request(device, getdevice, countof(getdevice), response, countof(response), request);
	future_t * f1 = usb_submit(request);
	future_get(f1);

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
		usb_control_request(device, setaddress, countof(setaddress), 0, 0, request);
		f1 = usb_submit(request);
		future_get(f1);
		device->dev = address;

		uint8_t * buf = malloc(response[0]);
		getdevice[6] = response[0];
		usb_control_request(device, getdevice, countof(getdevice), buf, response[0], request);
		f1 = usb_submit(request);
		future_get(f1);

		uint8_t getdescriptor[] = {0x80, 0x6, 0, 2, 0, 0, 0x9, 0};
		uint8_t config[64] = {0};
		usb_control_request(device, getdescriptor, countof(getdescriptor), config, getdescriptor[6], request);
		f1 = usb_submit(request);
		future_get(f1);

		usb_configuration_descriptor_t * configuration = usb_parse_configuration_descriptor_packet(0, config, getdescriptor[6]);
		getdescriptor[6] = configuration->totallength;
		usb_control_request(device, getdescriptor, countof(getdescriptor), config, getdescriptor[6], request);
		f1 = usb_submit(request);
		future_get(f1);
		configuration = usb_parse_configuration_descriptor_packet(0, config, getdescriptor[6]);

		uint8_t setconfig[] = {0x00, 0x9, configuration->configuration, 0x0, 0x0, 0x0, 0x0, 0x0};
		usb_control_request(device, setaddress, countof(setaddress), 0, 0, request);
		f1 = usb_submit(request);
		future_get(f1);
		device->configuration = configuration;

		/* Register the device with the device manager */
	}
}

static void usb_hub_enumerate(usb_hub_t * hub)
{
	int portcount = usb_hub_port_count(hub);

	for(int i=0; i<portcount; i++) {
		usb_hub_reset_port(hub, i);
		usb_device_t * device = usb_hub_get_device(hub, i);
		if (device && 0 == device->dev) {
			/* Get the device into an addressed state */
			device->hub = hub;
			usb_initialize_device(device);
		}
		hub->ports[i]=device;
	}
}

void usb_hub_device_enumerate(device_t * device)
{
	usb_hub_enumerate(device->ops->query(device, iid_usb_hub_t));
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

future_t * usb_submit(usb_request_t * request)
{
	return request->endpoint->device->hcd->ops->submit(request->endpoint->device->hcd, request);
}

void usb_test(usb_hub_t * hub)
{
	usb_hub_enumerate(hub);
}

char * usb_class_key(int class, int subclass)
{
	return device_key("usb:class:%x:%x", class, subclass);
}

char * usb_product_key(int vendor, int product)
{
	return device_key("usb:product:%x:%x", vendor, product);
}

void usb_init()
{
	device_driver_register(usb_class_key(9, 0), usb_hub_device_enumerate);
}

char iid_hcd_t[] = "USB Host Controller Device";
char iid_usb_hub_t[] = "USB Hub";
