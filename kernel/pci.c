#include "pci.h"

#if INTERFACE

#include <stdint.h>

typedef void (*pci_probe_callback)(uint8_t bus, uint8_t slot, uint8_t function);

#endif

static uint8_t pci_config_byte(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
	uint32_t reg = pci_config_read(bus, slot, function, offset & 0xfc);

	return (reg >> (8*(offset & 0x3))) & 0xFF;
}

static uint16_t pci_config_short(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset)
{
	uint32_t reg = pci_config_read(bus, slot, function, offset & 0xfc);

	return (reg >> (8*(offset & 0x3))) & 0xFFFF;
}

uint16_t pci_vendorid(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_short(bus, slot, function, 0);
}

uint16_t pci_deviceid(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_short(bus, slot, function, 2);
}

uint16_t pci_command(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_short(bus, slot, function, 4);
}

uint16_t pci_status(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_short(bus, slot, function, 6);
}

uint8_t pci_headertype(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_byte(bus, slot, function, 0xe);
}

void * pci_bar_map(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar)
{
	uint32_t reg = pci_config_read(bus, slot, function, 0x10 + bar*4);

	if (reg & 1) {
		/* I/O space */
		return pci_map_io(reg);
	} else {
		/* Get the size */
		pci_config_write(bus, slot, function, 0x10 + bar*4, ~0);
		uint32_t size = pci_config_read(bus, slot, function, 0x10 + bar*4);
		size &= ~0xf;
		size ^= ~0;
		size++;
		pci_config_write(bus, slot, function, 0x10 + bar*4, reg);

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

uint32_t pci_bar_base(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar)
{
	uint32_t reg = pci_config_read(bus, slot, function, 0x10 + bar*4);

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

uint32_t pci_bar_size(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar)
{
	uint32_t reg = pci_config_read(bus, slot, function, 0x10 + bar*4);
	uint32_t size = 0;
	uint32_t mask = (reg & 1) ? 0xFFFFFFFC : 0xFFFFFFF0;

	pci_config_write(bus, slot, function, 0x10 + bar*4, 0xffffffff);
	size = 1 + ~(pci_config_read(bus, slot, function, 0x10 + bar*4)&mask);
	pci_config_write(bus, slot, function, 0x10 + bar*4, reg);

	return size;
}

uint8_t pci_class(uint8_t bus, uint8_t slot, uint8_t function)
{
        return pci_config_byte(bus, slot, function, 0xb);
}

uint8_t pci_subclass(uint8_t bus, uint8_t slot, uint8_t function)
{
        return pci_config_byte(bus, slot, function, 0xa);
}

uint8_t pci_progif(uint8_t bus, uint8_t slot, uint8_t function)
{
        return pci_config_byte(bus, slot, function, 0x9);
}

uint8_t pci_irq(uint8_t bus, uint8_t slot, uint8_t function)
{
        return pci_config_byte(bus, slot, function, 0x3c);
}

uint8_t pci_secondary_bus(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_byte(bus, slot, function, 0x19);
}


void pci_probe_devfs(uint8_t bus, uint8_t device, uint8_t function)
{
	static GCROOT vnode_t * devfs = 0;

	if (0==devfs) {
		devfs = devfs_open();
	}

	char path[256];
	snprintf(path, countof(path), "bus/pci/%x/%x/%x", bus, device, function);
	vnode_newdir_hierarchy(devfs, path);
}

void pci_probe_print(uint8_t bus, uint8_t device, uint8_t function)
{
	uint8_t type = pci_headertype(bus, device, function) & 0x7f;

	kernel_printk("PCI %d, %d, %d - %x:%x\n",
		bus, device, function,
		pci_vendorid(bus, device, function), 
		pci_deviceid(bus, device, function));
	if (0 == type) {
		int bar;

		for(bar = 0; bar<6; bar++) {
			uint32_t size = pci_bar_size(bus, device, function, bar);
			if (size) {
				uint32_t base = pci_bar_base(bus, device, function, bar);
				kernel_printk("  Base address: %p (%x) %s\n", base, size, "" );
			}
		}
	}
}

typedef struct pci_probe_qualifier_t pci_probe_qualifier_t;
struct pci_probe_qualifier_t {
	uint16_t vendorid;
	uint16_t deviceid;
	uint8_t class;
	uint8_t subclass;
	uint8_t progif;
};

static void pci_scanbus(pci_probe_qualifier_t * qualifier, pci_probe_callback cb, uint8_t bus);
static void pci_probe_function(pci_probe_qualifier_t * qualifier, pci_probe_callback cb, uint8_t bus, uint8_t device, uint8_t function)
{
	uint8_t class = pci_class(bus, device, function);
	uint8_t subclass = pci_subclass(bus, device, function);
	uint8_t progif = pci_progif(bus, device, function);

	if (0xff != qualifier->progif && qualifier->progif != progif) {
		return;
	}
	if (0xff != qualifier->class && qualifier->class != class) {
		return;
	}
	if (0xff != qualifier->subclass && qualifier->subclass != subclass) {
		return;
	}
	if (0xffff != qualifier->vendorid && qualifier->vendorid != pci_vendorid(bus, device, function)) {
		return;
	}
	if (0xffff != qualifier->deviceid && qualifier->deviceid != pci_deviceid(bus, device, function)) {
		return;
	}

	cb(bus, device, function);

	if (0x6 == class && 0x4 == subclass) {
		uint8_t bus2 = pci_secondary_bus(bus, device, function);
		pci_scanbus(qualifier, cb, bus2);
	}
}

static void pci_probe_device(pci_probe_qualifier_t * qualifier, pci_probe_callback cb, uint8_t bus, uint8_t device)
{
	if (0xFFFF == pci_vendorid(bus, device, 0)) {
		return;
	}

	pci_probe_function(qualifier, cb, bus, device, 0);

	if (pci_headertype(bus, device, 0) & 0x80) {
		/* Multi-function device */
		int i;

		for(i=1; i<8; i++) {
			if (0xFFFF != pci_vendorid(bus, device, i)) {
				pci_probe_function(qualifier, cb, bus, device, i);
			}
		}
	}
}

static void pci_scanbus(pci_probe_qualifier_t * qualifier, pci_probe_callback cb, uint8_t bus)
{
	uint8_t device;

	for(device=0; device<32; device++) {
		pci_probe_device(qualifier, cb, bus, device);
	}
}

static void pci_scan_qualified(pci_probe_qualifier_t * qualifier, pci_probe_callback cb)
{
	if (0x80 & pci_headertype(0, 0, 0)) {
		int function;

		for(function=0; function<8; function++) {
			if (0xFFFF != pci_vendorid(0, 0, function)) {
				break;
			}
			pci_scanbus(qualifier, cb, function);
		}
	} else {
		pci_scanbus(qualifier, cb, 0);
	}
}

void pci_scan(pci_probe_callback cb)
{
	pci_probe_qualifier_t qualifier[1] = {{.class = 0xff, .subclass = 0xff, .progif = 0xff, .vendorid = 0xffff, .deviceid = 0xffff}};
	pci_scan_qualified(qualifier, cb);
}

void pci_scan_class(pci_probe_callback cb, uint8_t class, uint8_t subclass, uint8_t progif, uint16_t vendorid, uint16_t deviceid)
{
	pci_probe_qualifier_t qualifier[1] = {{.class = class, .subclass = subclass, .progif = progif, .vendorid = vendorid, .deviceid = deviceid}};
	pci_scan_qualified(qualifier, cb);
}
