#include <stdint.h>

#include "pci.h"

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

uint16_t pci_vendor(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_short(bus, slot, function, 0);
}

uint16_t pci_device(uint8_t bus, uint8_t slot, uint8_t function)
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

uint32_t pci_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t bar)
{
	return pci_config_read(bus, slot, function, 0x10 + bar*4);
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

uint8_t pci_secondary_bus(uint8_t bus, uint8_t slot, uint8_t function)
{
	return pci_config_byte(bus, slot, function, 0x19);
}

void pci_probe_function(uint8_t bus, uint8_t device, uint8_t function)
{
	uint8_t class = pci_class(bus, device, function);
	uint8_t subclass = pci_subclass(bus, device, function);
	uint8_t type = pci_headertype(bus, device, function) & 0x7f;

	kernel_printk("PCI %d, %d, %d - %x:%x\n",
		bus, device, function,
		pci_vendor(bus, device, function), 
		pci_device(bus, device, function));
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

	if (0x6 == class && 0x4 == subclass) {
		uint8_t bus = pci_secondary_bus(bus, device, function);
		pci_scanbus(bus);
	}
}

void pci_probe_device(uint8_t bus, uint8_t device)
{

	if (0xFFFF == pci_vendor(bus, device, 0)) {
		return;
	}

	pci_probe_function(bus, device, 0);

	if (pci_headertype(bus, device, 0) & 0x80) {
		/* Multi-function device */
		int i;

		for(i=1; i<8; i++) {
			if (0xFFFF != pci_vendor(bus, device, i)) {
				pci_probe_function(bus, device, i);
			}
		}
	}
}

void pci_scanbus(uint8_t bus)
{
	uint8_t device;

	for(device=0; device<32; device++) {
		pci_probe_device(bus, device);
	}
}

void pci_scan()
{
	if (0x80 & pci_headertype(0, 0, 0)) {
		int function;

		for(function=0; function<8; function++) {
			if (0xFFFF != pci_vendor(0, 0, function)) {
				break;
			}
			pci_scanbus(function);
		}
	} else {
		pci_scanbus(0);
	}
}
