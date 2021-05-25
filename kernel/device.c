#include "device.h"

#if INTERFACE

struct device_ops_t
{
	void (*enumerate)(device_t * device);
};

struct device_t
{
	device_ops_t * ops;
};

typedef int device_type_t;

#endif

static spin_t typelock[1];
static GCROOT map_t * types;

device_type_t device_type(char * type)
{
	device_type_t device_type;

	SPIN_AUTOLOCK(typelock) {
		static device_type_t next = 0;
		if (0 == types) {
			types = treap_new(map_strcmp);
		}

		device_type = map_getpi(types, type);
		if (0 == type) {
			device_type = ++next;
			map_putpi(types, type, device_type);
		}
	}

	return device_type;
}

void device_enumerate(device_t * device)
{
	device->ops->enumerate(device);
}


device_t * device_probe(device_t * parent, void * p)
{
	map_compound_key_t * key = p;
}



void device_queue(device_t * parent, void * key, device_t * device)
{
}
