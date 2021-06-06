#include <stdarg.h>
#include "device.h"

#if INTERFACE

struct device_ops_t
{
	void (*enumerate)(device_t * device);
};

typedef int device_type_t;

typedef void (*device_probe_t)(device_t * device);

struct device_t
{
	device_ops_t * ops;

	device_type_t type;
	device_t * parent;
	map_t * children;
};

#endif

typedef struct device_list_t device_list_t;
struct device_list_t {
	char * key;
	device_t * device;

	device_list_t * next;
	device_list_t * prev;
};

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

void device_init(device_t * device, device_ops_t * ops, device_type_t type, device_t * parent)
{
	device->ops = ops;
	device->type = type;
	device->children = treap_new(0);
	device->parent = parent;

	if (parent) {
		map_putpp(parent->children, device, device);
	}
}

static monitor_t lock[1];
static GCROOT map_t * unclaimed;
static GCROOT map_t * index;
static GCROOT map_t * probes;
static GCROOT device_list_t * queue;

void device_manager_init()
{
	if (0 == unclaimed) {
		unclaimed = treap_new(0);
		index = treap_new(map_strcmp);
		probes = treap_new(map_strcmp);
	}
}

void device_queue(device_t * device, ...)
{
	va_list ap;
	va_start(ap, device);

	MONITOR_AUTOLOCK(lock) {
		map_putpp(unclaimed, device, device);
		char * key = va_arg(ap, char*);
		while(key) {
			device_list_t temp_item = {.key=key, .device=device};
			device_list_t * item = mclone(&temp_item);
			LIST_APPEND(queue, item);
			key = va_arg(ap, char*);
		}
		monitor_broadcast(lock);
	}

	va_end(ap);
}

void device_probe_walk(const void * const p, void * key, void * data)
{
	device_probe_t probe = key;
	device_t * device = (device_t*)p;
	if (map_getpp(unclaimed, device)) {
		probe(device);
	}
}

void device_probe_unclaimed()
{
	MONITOR_AUTOLOCK(lock) {
		device_list_t * next = queue;
		while(next) {
			device_list_t * item = next;
			LIST_NEXT(queue, next);

			if (map_getpp(unclaimed, item->device)) {
				map_t * probeset = map_getpp(probes, item->key);
				if (probeset) {
					map_walkpp(probeset, device_probe_walk, item->device);
				}
			} else {
				/* Claimed device, remove this item */
				LIST_DELETE(queue, item);
			}
		}
	}
}

void device_claim(device_t * device)
{
	MONITOR_AUTOLOCK(lock) {
		if (0 == map_removepp(unclaimed, device)) {
			// Already claimed!
			kernel_panic("Claiming already claimed device");
		}
	}
}

void device_driver_register(char * key, device_probe_t probe)
{
	MONITOR_AUTOLOCK(lock) {
		map_t * probeset = map_getpp(probes, key);
		if (0 == probeset) {
			probeset = treap_new(0);
			map_putpp(probes, key, probeset);
		}
		map_putpp(probeset, probe, probe);
	}
}
