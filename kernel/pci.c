#include "pci.h"

#if INTERFACE

#include <stdint.h>

typedef void (*pci_probe_callback)(pci_device_t * pcidev);

struct pci_device_t {
	/* Generic device tree */
	device_t device;

	/* PCI specific information */
	uint8_t bus;
	uint8_t slot;
	uint8_t function;
};

#endif

static uint8_t pci_config_byte(pci_device_t * pcidev, uint8_t offset)
{
	uint32_t reg = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, offset & 0xfc);

	return (reg >> (8*(offset & 0x3))) & 0xFF;
}

static uint16_t pci_config_short(pci_device_t * pcidev, uint8_t offset)
{
	uint32_t reg = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, offset & 0xfc);

	return (reg >> (8*(offset & 0x3))) & 0xFFFF;
}

uint16_t pci_vendorid(pci_device_t * pcidev)
{
	return pci_config_short(pcidev, 0);
}

uint16_t pci_deviceid(pci_device_t * pcidev)
{
	return pci_config_short(pcidev, 2);
}

uint16_t pci_command(pci_device_t * pcidev)
{
	return pci_config_short(pcidev, 4);
}

uint16_t pci_status(pci_device_t * pcidev)
{
	return pci_config_short(pcidev, 6);
}

uint8_t pci_headertype(pci_device_t * pcidev)
{
	return pci_config_byte(pcidev, 0xe);
}

void * pci_bar_map(pci_device_t * pcidev, uint8_t bar)
{
	uint32_t reg = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4);

	if (reg & 1) {
		/* I/O space */
		return pci_map_io(reg);
	} else {
		/* Get the size */
		pci_config_write(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4, ~0);
		uint32_t size = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4);
		size &= ~0xf;
		size ^= ~0;
		size++;
		pci_config_write(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4, reg);

		int type = (reg >> 1) & 0x3;
		int pgoff = reg & (ARCH_PAGE_SIZE-1) & ~0x3;
		switch(type) {
		case 0:
			return PTR_BYTE_ADDRESS(vm_map_paddr(reg >> ARCH_PAGE_SIZE_LOG2, size + pgoff), pgoff);
		default:
			KTHROWF(NotImplementedException, "Memory mapping of PCI this type of BAR not supported: %d", type);
		}
	}
}

uint32_t pci_bar_base(device_t * device, uint8_t bar)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	uint32_t reg = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4);

	if (reg & 1) {
		/* I/O space */
		return reg & 0xFFFFFFFC;
	} else {
		int type = (reg >> 1) & 0x3;
		if (0 == type) {
			return reg & 0xFFFFFFF0;
		}
	}
	return reg;
}

uint32_t pci_bar_size(device_t * device, uint8_t bar)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	uint32_t reg = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4);

	pci_config_write(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4, 0xffffffff);
	uint32_t size = pci_config_read(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4);
	size &= (reg & 1) ? 0xFFFFFFFC : 0xFFFFFFF0;
	size ^= ~0;
	size++;
	pci_config_write(pcidev->bus, pcidev->slot, pcidev->function, 0x10 + bar*4, reg);

	return size;
}

uint8_t pci_class(pci_device_t * pcidev)
{
        return pci_config_byte(pcidev, 0xb);
}

uint8_t pci_subclass(pci_device_t * pcidev)
{
        return pci_config_byte(pcidev, 0xa);
}

uint8_t pci_progif(pci_device_t * pcidev)
{
        return pci_config_byte(pcidev, 0x9);
}

uint8_t pci_irq(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
        return pci_config_byte(pcidev, 0x3c);
}

uint8_t pci_secondary_bus(pci_device_t * pcidev)
{
	return pci_config_byte(pcidev, 0x19);
}


void pci_probe_devfs(pci_device_t * pcidev)
{
	static GCROOT vnode_t * devfs = 0;

	if (0==devfs) {
		devfs = devfs_open();
	}

	char path[256];
	snprintf(path, countof(path), "bus/pci/%x/%x/%x", pcidev->bus, pcidev->slot, pcidev->function);
	vnode_newdir_hierarchy(devfs, path);
}

void pci_probe_print(pci_device_t * pcidev)
{
	uint8_t type = pci_headertype(pcidev) & 0x7f;

	kernel_printk("PCI %d, %d, %d - %x:%x\n",
		pcidev->bus, pcidev->slot, pcidev->function,
		pci_vendorid(pcidev), 
		pci_deviceid(pcidev));
	if (0 == type) {
		int bar;

		for(bar = 0; bar<6; bar++) {
			uint32_t size = pci_bar_size(pcidev, bar);
			if (size) {
				uint32_t base = pci_bar_base(pcidev, bar);
				kernel_printk("  Base address: %p (%x) %s\n", base, size, "" );
			}
		}
	}
}


static void pci_scanbus(device_t * parent, pci_device_t * pcidev);
static int pci_bus_enumerate(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	pci_device_t pcibus2[1] = {{.bus=pci_secondary_bus(pcidev), .slot=0, .function=0}};
	pci_scanbus(device, pcibus2);

	return 1;
}

static void pci_device_enumerate(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
}

static device_ops_t pci_ops[1] = {{.enumerate=pci_device_enumerate}};

static char * pci_device_key( char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	char * key;
	ARENA_AUTOSTATE(NULL) {
		char * tkey = tmalloc(128);
		vsnprintf(tkey, 128, fmt, ap);
		key = strdup(tkey);
	}

	va_end(ap);

	return key;
}

static device_type_t pci_type = 0;

char * pci_progif_key(uint8_t class, uint8_t subclass, uint8_t progif)
{
	return pci_device_key("%d:progif:%d:%d:%d", pci_type, class, subclass, progif);
}

char * pci_class_key(uint8_t class, uint8_t subclass)
{
	return pci_device_key("%d:class:%d:%d", pci_type, class, subclass);
}

char * pci_deviceid_key(uint16_t vendorid, uint16_t deviceid)
{
	return pci_device_key("%d:vendor:%d:%d", pci_type, vendorid, deviceid);
}

static char * pci_device_progif_key(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	uint8_t class = pci_class(pcidev);
	uint8_t subclass = pci_subclass(pcidev);
	uint8_t progif = pci_progif(pcidev);
	return pci_progif_key(class, subclass, progif);
}

static char * pci_device_class_key(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	uint8_t class = pci_class(pcidev);
	uint8_t subclass = pci_subclass(pcidev);
	return pci_class_key(class, subclass);
}

static char * pci_device_deviceid_key(device_t * device)
{
	pci_device_t * pcidev = container_of(device, pci_device_t, device);
	uint16_t vendorid = pci_vendorid(pcidev);
	uint16_t deviceid = pci_deviceid(pcidev);
	return pci_deviceid_key(vendorid, deviceid);
}

static void pci_probe_function(device_t * parent, pci_device_t * pcidevtmp)
{
	pci_device_t * pcidev = mclone(pcidevtmp);
	device_t * device = &pcidev->device;

	/* Link into the device tree */
	device_init(device, pci_ops, pci_type, parent);

	/* Queue the device for driver probing */
	device_queue(device, pci_device_deviceid_key(device), pci_device_class_key(device), pci_device_progif_key(device), NULL);
}

static void pci_probe_slot(device_t * parent, pci_device_t * pcidev)
{
	if (0xFFFF == pci_vendorid(pcidev)) {
		return;
	}

	pcidev->function = 0;
	pci_probe_function(parent, pcidev);

	if (pci_headertype(pcidev) & 0x80) {
		/* Multi-function device */
		for(pcidev->function = 1; pcidev->function<8; pcidev->function++) {
			if (0xFFFF != pci_vendorid(pcidev)) {
				pci_probe_function(parent, pcidev);
			}
		}
	}
}

static void pci_scanbus(device_t * parent, pci_device_t * pcidev)
{
	for(pcidev->slot=0; pcidev->slot<32; pcidev->slot++) {
		pci_probe_slot(parent, pcidev);
	}
}

static void pci_scan_root(device_t * parent)
{
	pci_device_t pcidev[1] = {{.bus=0, .slot=0, .function=0}};
	if (0x80 & pci_headertype(pcidev)) {
		int function;

		for(function=0; function<8; function++) {
			if (0xFFFF != pci_vendorid(pcidev)) {
				break;
			}
			pci_scanbus(parent, pcidev);
		}
	} else {
		pci_scanbus(parent, pcidev);
	}
}

static void pci_scan(device_t * parent)
{
}

void pci_init(device_t * parent)
{
	pci_type = device_type("bus/pci");
	device_driver_register(pci_class_key(6, 4), pci_bus_enumerate);
	pci_scan_root(parent);
}
