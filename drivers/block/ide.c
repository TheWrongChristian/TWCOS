#include "ide.h"

#if INTERFACE

#include <sys/types.h>
#include <stdint.h>

/* Status */
#define ATA_SR_BSY     0x80    /* Busy */
#define ATA_SR_DRDY    0x40    /* Drive ready */
#define ATA_SR_DF      0x20    /* Drive write fault */
#define ATA_SR_DSC     0x10    /* Drive seek complete */
#define ATA_SR_DRQ     0x08    /* Data request ready */
#define ATA_SR_CORR    0x04    /* Corrected data */
#define ATA_SR_IDX     0x02    /* Index */
#define ATA_SR_ERR     0x01    /* Error */

/* Errors */
#define ATA_ER_BBK      0x80    /* Bad block */
#define ATA_ER_UNC      0x40    /* Uncorrectable data */
#define ATA_ER_MC       0x20    /* Media changed */
#define ATA_ER_IDNF     0x10    /* ID mark not found */
#define ATA_ER_MCR      0x08    /* Media change request */
#define ATA_ER_ABRT     0x04    /* Command aborted */
#define ATA_ER_TK0NF    0x02    /* Track 0 not found */
#define ATA_ER_AMNF     0x01    /* No address mark */

/* Commands */
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

/* ATAPI Commands */
#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

/* ATA Identify data */
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

/* Interface type */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/* ATA registers as offsets from base address */
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

/* Channel */
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01
 
/* Direction */
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01


typedef struct idechannel_t idechannel_t;
typedef struct idedevice_t idedevice_t;
typedef struct idecontroller_t idecontroller_t;

#endif
struct idechannel_t {
	unsigned short base;  /* I/O Base. */
	unsigned short ctrl;  /* Control Base */
	unsigned short bmide; /* Bus Master IDE */
	unsigned char  nIEN;  /* nIEN (No Interrupt); */
};

struct idedevice_t {
	dev_t dev;
	idechannel_t * channel;
	unsigned char  drive;       /* 0 (Master Drive) or 1 (Slave Drive). */
	unsigned short type;        /* 0: ATA, 1:ATAPI. */
	unsigned short signature;   /* Drive Signature */
	unsigned short capabilities;/* Features. */
	unsigned int   commandSets; /* Command Sets Supported. */
	unsigned int   size;        /* Size in Sectors. */
	unsigned char  model[41];   /* Model in string. */
};

struct idecontroller_t {
	idechannel_t channels[2];

	idedevice_t devices[4];
};

monitor_t idelock[1];

void ide_intr(int irq)
{
	MONITOR_AUTOLOCK(idelock) {
		monitor_broadcast(idelock);
	}
}

idecontroller_t * ide_initialize(uintptr_t bar0, uintptr_t bar1, uintptr_t bar2, uintptr_t bar3, uintptr_t bar4)
{
	idecontroller_t * ide = calloc(1, sizeof(*ide));

	// 1- Detect I/O Ports which interface IDE Controller:
	ide->channels[ATA_PRIMARY  ].base  = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
	ide->channels[ATA_PRIMARY  ].ctrl  = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
	ide->channels[ATA_SECONDARY].base  = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
	ide->channels[ATA_SECONDARY].ctrl  = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
	ide->channels[ATA_PRIMARY  ].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
	ide->channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE

	add_irq(14, ide_intr);
	add_irq(15, ide_intr);

	return ide;
}

uint8_t ide_read(idechannel_t * channel, int reg)
{
	if (reg > ATA_REG_COMMAND && reg < ATA_REG_ALTSTATUS) {
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);
	}

	uint8_t result;
	switch(reg) {
	case ATA_REG_ERROR:
	case ATA_REG_SECCOUNT0:
	case ATA_REG_LBA0:
	case ATA_REG_LBA1:
	case ATA_REG_LBA2:
	case ATA_REG_HDDEVSEL:
	case ATA_REG_STATUS:
		result = inb(channel->base+reg);
		break;
	case ATA_REG_SECCOUNT1:
	case ATA_REG_LBA3:
	case ATA_REG_LBA4:
	case ATA_REG_LBA5:
		result = inb(channel->base+reg-6);
		break;
	case ATA_REG_ALTSTATUS:
	case ATA_REG_DEVADDRESS:
		result = inb(channel->ctrl+reg-0xc);
		break;
	default:
		break;
	}

	if (reg > ATA_REG_COMMAND && reg < ATA_REG_ALTSTATUS) {
		ide_write(channel, ATA_REG_CONTROL, channel->nIEN);
	} 
	return result;
}

void ide_write(idechannel_t * channel, int reg, uint8_t value)
{
	if (reg > ATA_REG_COMMAND && reg < ATA_REG_ALTSTATUS) {
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);
	} 

	switch(reg) {
	case ATA_REG_DATA:
	case ATA_REG_FEATURES:
	case ATA_REG_SECCOUNT0:
	case ATA_REG_LBA0:
	case ATA_REG_LBA1:
	case ATA_REG_LBA2:
	case ATA_REG_HDDEVSEL:
	case ATA_REG_COMMAND:
		outb(channel->base+reg, value);
		break;
	case ATA_REG_SECCOUNT1:
	case ATA_REG_LBA3:
	case ATA_REG_LBA4:
	case ATA_REG_LBA5:
		outb(channel->base+reg-6, value);
		break;
	case ATA_REG_CONTROL:
		outb(channel->ctrl+reg-6, value);
		break;
	}

	if (reg > ATA_REG_COMMAND && reg < ATA_REG_ALTSTATUS) {
		ide_write(channel, ATA_REG_CONTROL, channel->nIEN);
	} 
}

void ide_write_pio(idechannel_t * channel, void * buf, size_t bufsize)
{
	uint16_t * p16 = buf;
	size_t count=bufsize/sizeof(*p16);

	for(int i=0; i<count; i++) {
		outw(channel->base+ATA_REG_DATA, *p16++);
	}
}

void ide_read_pio(idechannel_t * channel, void * buf, size_t bufsize)
{
	uint16_t * p16 = buf;
	size_t count=bufsize/sizeof(*p16);

	for(int i=0; i<count; i++) {
		*p16++ = inw(channel->base+ATA_REG_DATA);
	}
}

#define BYTE(lba, byte) ((lba>>(byte*8)) & 0xff)
void ide_address(idechannel_t * channel, int slave, off64_t lba, size_t count)
{
	if (lba >= 1<<28) {
		// LBA48 mode
		ide_write(channel, ATA_REG_SECCOUNT1, BYTE(count, 1));
		ide_write(channel, ATA_REG_LBA3, BYTE(lba, 3));
		ide_write(channel, ATA_REG_LBA4, BYTE(lba, 4));
		ide_write(channel, ATA_REG_LBA5, BYTE(lba, 5));
		ide_write(channel, ATA_REG_SECCOUNT0, BYTE(count, 0));
		ide_write(channel, ATA_REG_LBA0, BYTE(lba, 0));
		ide_write(channel, ATA_REG_LBA1, BYTE(lba, 1));
		ide_write(channel, ATA_REG_LBA2, BYTE(lba, 2));
		// Select drive
		if (slave) {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xf0);
		} else {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xe0);
		}
	} else {
		ide_write(channel, ATA_REG_LBA0, BYTE(lba, 0));
		ide_write(channel, ATA_REG_LBA1, BYTE(lba, 1));
		ide_write(channel, ATA_REG_LBA2, BYTE(lba, 2));

		// Select drive, and any remaining LBA bits in LBA28 mode
		if (slave) {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xf0 | BYTE(lba, 3));
		} else {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xe0 | BYTE(lba, 3));
		}
	}
}

void ide_delay400(idechannel_t * channel)
{
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
}

void ide_wait(idechannel_t * channel)
{
	MONITOR_AUTOLOCK(idelock) {
		uint8_t status;
		do {
			status = ide_read(channel, ATA_REG_ALTSTATUS);
			if (status & 0x80) {
				monitor_wait(idelock);
			}
		} while(status & 0x80);
	}
}

void ide_drive_identify(idechannel_t * channel, int slave)
{
	uint8_t buf[256];

	ide_address(channel, slave, 0, 256);
	ide_wait(channel);
	ide_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ide_delay400(channel);
	ide_wait(channel);
	ide_read_pio(channel, buf, countof(buf));
}

void ide_drive_read_sector(idechannel_t * channel, int slave, off64_t lba, void * buf, size_t bufsize)
{
	ide_address(channel, slave, lba, 1);
	ide_wait(channel);
	ide_write(channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO);
	ide_delay400(channel);
	ide_wait(channel);
	ide_read_pio(channel, buf, bufsize);
}

void ide_probe(uint8_t bus, uint8_t slot, uint8_t function)
{
	if (1==pci_class(bus, slot, function) && 1==pci_subclass(bus, slot, function)) {
		uint8_t irq = pci_irq(bus, slot, function);
		switch(pci_progif(bus, slot, function)) {
		case 0x8a:
			break;
		case 0x80:
			irq = 15;
			break;
		}

		uintptr_t bar0 = pci_bar_base(bus, slot, function, 0);
		uintptr_t bar1 = pci_bar_base(bus, slot, function, 1);
		uintptr_t bar2 = pci_bar_base(bus, slot, function, 2);
		uintptr_t bar3 = pci_bar_base(bus, slot, function, 3);
		uintptr_t bar4 = pci_bar_base(bus, slot, function, 4);

		idecontroller_t * ide = ide_initialize(bar0, bar1, bar2, bar3, bar4);

		static uint8_t buf[512];
		ide_drive_identify(ide->channels, 0);
		ide_drive_read_sector(ide->channels, 0, 0, buf, countof(buf));
	}
}

void ide_pciscan()
{
	pci_scan(ide_probe);
}
