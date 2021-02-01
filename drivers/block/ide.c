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

#define	IDE_SECTORSIZE_LOG2 9
#define	IDE_SECTORSIZE (1<<IDE_SECTORSIZE_LOG2)


typedef struct idechannel_t idechannel_t;
typedef struct idedevice_t idedevice_t;
typedef struct idecontroller_t idecontroller_t;

#endif

static void ide_intr(void * p);
static void ide_reset(idechannel_t * channel);
static unsigned ide_field_get(const uint8_t * buf, size_t bufsize, int offset, size_t size);
static void ide_disk_submit( dev_t * dev, buf_op_t * op );
static void ide_do_op( dev_t * dev, buf_op_t * op );
static void ide_thread(idechannel_t * channel);
static void ide_probe_channel(idechannel_t * channel);
static idecontroller_t * ide_initialize(uintptr_t bar0, uintptr_t bar1, uintptr_t bar2, uintptr_t bar3, uintptr_t bar4, uint8_t irq);
static uint8_t ide_read(idechannel_t * channel, int reg);
static void ide_write(idechannel_t * channel, int reg, uint8_t value);
static void ide_write_pio(idechannel_t * channel, void * buf, size_t bufsize);
static void ide_read_pio(idechannel_t * channel, void * buf, size_t bufsize);
static void ide_address(idechannel_t * channel, int slave, off64_t lba, size_t count);
static void ide_delay400(idechannel_t * channel);
static uint8_t ide_wait(idechannel_t * channel, int polling);
static void ide_command(idechannel_t * channel, uint8_t command);
static void ide_drive_identify(idechannel_t * channel, int slave, uint8_t * buf, size_t bufsize);
static void ide_drive_transfer_sectors(idechannel_t * channel, int slave, off64_t lba, int write, void * buf, size_t bufsize);
static void ide_probe(uint8_t bus, uint8_t slot, uint8_t function);

struct idedevice_t {
	dev_t dev;
	idechannel_t * channel;
	int drive;       /* 0 (Master Drive) or 1 (Slave Drive). */
	int type;        /* 0: ATA, 1:ATAPI. */
	unsigned signature;   /* Drive Signature */
	unsigned capabilities;/* Features. */
	unsigned commandSets; /* Command Sets Supported. */
	off64_t size;        /* Size in Sectors. */
	char model[41];   /* Model in string. */
};

struct idechannel_t {
	interrupt_monitor_t * lock;
	thread_t * thread;
	buf_op_t * op;
	int base;  /* I/O Base. */
	int ctrl;  /* Control Base */
	int bmide; /* Bus Master IDE */
	int polling;  /* polling (No Interrupt); */

	idedevice_t devices[2];
};

struct idecontroller_t {
	idechannel_t channels[2];
};

static void ide_intr(void * p)
{
	idechannel_t * channel = p;
	interrupt_monitor_broadcast(channel->lock);
}

static void ide_reset(idechannel_t * channel)
{
	ide_write(channel, ATA_REG_CONTROL, 4);
	timer_sleep(5);
	ide_write(channel, ATA_REG_CONTROL, 0);
}

static unsigned ide_field_get(const uint8_t * buf, size_t bufsize, int offset, size_t size)
{
	unsigned retval=0;

	check_int_bounds(offset, 0, bufsize-size, "Read beyond end of buffer");
	for(int i=0; i<size; i++) {
		retval |= buf[offset+i] << 8*i;
	}

	return retval;
}


static void ide_disk_submit( dev_t * dev, buf_op_t * op )
{
	idedevice_t * device = container_of(dev, idedevice_t, dev);
	INTERRUPT_MONITOR_AUTOLOCK(device->channel->lock) {
		while(device->channel->op) {
			interrupt_monitor_wait_timeout(device->channel->lock, 1000000);
		}

		device->channel->op = op;
		interrupt_monitor_broadcast(device->channel->lock);
	}
}

static void ide_do_op( dev_t * dev, buf_op_t * op )
{
	MONITOR_AUTOLOCK(op->lock) {
		idedevice_t * device = container_of(dev, idedevice_t, dev);
		off64_t sector = op->offset >> IDE_SECTORSIZE_LOG2;

		ide_drive_transfer_sectors(device->channel, device->drive, sector, op->write, op->p, op->size);

		op->status = DEV_BUF_OP_COMPLETE;
		monitor_broadcast(op->lock);
	}
}

static void ide_thread(idechannel_t * channel)
{
	while(1) {
		INTERRUPT_MONITOR_AUTOLOCK(channel->lock) {
			while(0 == channel->op) {
				interrupt_monitor_wait(channel->lock);
			}
			dev_t * dev = channel->op->dev;
			ide_do_op(dev, channel->op);

			/* Done with this op */
			channel->op = 0;
		}
	}
}

static void ide_probe_channel(idechannel_t * channel)
{
	INTERRUPT_MONITOR_AUTOLOCK(channel->lock) {
		static uint8_t buf[IDE_SECTORSIZE];
		char devfsname[64];
		int needthread = 0;
		snprintf(devfsname, sizeof(devfsname), "disk/ide/%x", channel->base);
		vnode_t * devfs = devfs_open();
		for(int i=0; i<2; i++) {
			idedevice_t * device = channel->devices+i;
		
			device->channel = channel;	
			ide_address(channel, i, 0, 0);
			uint8_t status = ide_wait(channel, channel->polling);
			if (0==status) {
				/* No drive */
				memset(device, 0, sizeof(*device));
			} else if (status & (ATA_SR_DRDY|ATA_SR_DRQ)) {
				ide_drive_identify(channel, i, buf, countof(buf));
				/* Check for ATAPI device */
				status = ide_wait(channel, channel->polling);
				if (0x14 == ide_read(channel, ATA_REG_LBA1) && 0xEB == ide_read(channel, ATA_REG_LBA2)) {
					static dev_ops_t ide_atapi_ops;
					/* ATAPI device */
					device->type = IDE_ATAPI;
					device->dev.ops = &ide_atapi_ops;
				} else {
					static dev_ops_t ide_disk_ops = { .submit = ide_disk_submit };
					device->size = ide_field_get(buf, sizeof(buf), ATA_IDENT_MAX_LBA, 4);
					device->type = IDE_ATA;
					device->dev.ops = &ide_disk_ops;
				}
				vnode_t * vnode = dev_vnode(&device->dev);
				vnode_t * dir = vnode_newdir_hierarchy(devfs, devfsname);
				vnode_put_vnode(dir, i ? "slave" : "master", vnode);
				needthread = 1;
			} else {
				/* Some error */
			}
		}
		if (needthread) {
			/* Processing thread */
			thread_t * thread = thread_fork();
			if (thread) {
				channel->thread = thread;
			} else {
				thread_set_name(0, "IDE async thread");
				ide_thread(channel);
			}
		}
	}
}

static idecontroller_t * ide_initialize(uintptr_t bar0, uintptr_t bar1, uintptr_t bar2, uintptr_t bar3, uintptr_t bar4, uint8_t irq)
{
	idecontroller_t * ide = calloc(1, sizeof(*ide));

	// 1- Detect I/O Ports which interface IDE Controller:
	ide->channels[ATA_PRIMARY  ].lock = interrupt_monitor_irq(14);
	ide->channels[ATA_PRIMARY  ].base  = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
	ide->channels[ATA_PRIMARY  ].ctrl  = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
	ide->channels[ATA_PRIMARY  ].polling = 0;

	ide->channels[ATA_SECONDARY].lock = interrupt_monitor_irq(15);
	ide->channels[ATA_SECONDARY].base  = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
	ide->channels[ATA_SECONDARY].ctrl  = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
	ide->channels[ATA_SECONDARY].polling = 0;

	ide->channels[ATA_PRIMARY  ].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
	ide->channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE

#if 0
	add_irq(14, ide_intr);
	add_irq(15, ide_intr);
#endif
	intr_add(14, ide_intr, ide->channels);
	intr_add(15, ide_intr, ide->channels+1);

	KTRY {
		ide_reset(ide->channels);
		ide_probe_channel(ide->channels);
	} KCATCH(TimeoutException) {
	}

	KTRY {
		ide_reset(ide->channels+1);
		ide_probe_channel(ide->channels+1);
	} KCATCH(TimeoutException) {
	}

	return ide;
}

static uint8_t ide_read(idechannel_t * channel, int reg)
{
	uint8_t result;
	switch(reg) {
	case ATA_REG_ERROR:
	case ATA_REG_SECCOUNT0:
	case ATA_REG_LBA0:
	case ATA_REG_LBA1:
	case ATA_REG_LBA2:
	case ATA_REG_HDDEVSEL:
	case ATA_REG_STATUS:
		result = isa_inb(channel->base+reg);
		break;
	case ATA_REG_SECCOUNT1:
	case ATA_REG_LBA3:
	case ATA_REG_LBA4:
	case ATA_REG_LBA5:
		result = isa_inb(channel->base+reg-6);
		break;
	case ATA_REG_ALTSTATUS:
	case ATA_REG_DEVADDRESS:
		result = isa_inb(channel->ctrl+reg-0xc);
		break;
	default:
		break;
	}

	return result;
}

static void ide_write(idechannel_t * channel, int reg, uint8_t value)
{
	switch(reg) {
	case ATA_REG_DATA:
	case ATA_REG_FEATURES:
	case ATA_REG_SECCOUNT0:
	case ATA_REG_LBA0:
	case ATA_REG_LBA1:
	case ATA_REG_LBA2:
	case ATA_REG_HDDEVSEL:
	case ATA_REG_COMMAND:
		isa_outb(channel->base+reg, value);
		break;
	case ATA_REG_SECCOUNT1:
	case ATA_REG_LBA3:
	case ATA_REG_LBA4:
	case ATA_REG_LBA5:
		isa_outb(channel->base+reg-6, value);
		break;
	case ATA_REG_CONTROL:
		isa_outb(channel->ctrl+reg-6, value);
		break;
	}
}

static void ide_write_pio(idechannel_t * channel, void * buf, size_t bufsize)
{
	uint16_t * p16 = buf;
	size_t count=bufsize/sizeof(*p16);

	for(int i=0; i<count; i++) {
		isa_outw(channel->base+ATA_REG_DATA, *p16++);
	}
}

static void ide_read_pio(idechannel_t * channel, void * buf, size_t bufsize)
{
	uint16_t * p16 = buf;
	size_t count=bufsize/sizeof(*p16);

	for(int i=0; i<count; i++) {
		*p16++ = isa_inw(channel->base+ATA_REG_DATA);
	}
}

#define BYTE(lba, byte) ((lba>>(byte*8)) & 0xff)
static void ide_address(idechannel_t * channel, int slave, off64_t lba, size_t count)
{
	if (lba >= 1<<28) {
		// Select drive
		if (slave) {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xf0);
		} else {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xe0);
		}
		// LBA48 mode
		ide_write(channel, ATA_REG_SECCOUNT1, BYTE(count, 1));
		ide_write(channel, ATA_REG_LBA3, BYTE(lba, 3));
		ide_write(channel, ATA_REG_LBA4, BYTE(lba, 4));
		ide_write(channel, ATA_REG_LBA5, BYTE(lba, 5));
		ide_write(channel, ATA_REG_SECCOUNT0, BYTE(count, 0));
		ide_write(channel, ATA_REG_LBA0, BYTE(lba, 0));
		ide_write(channel, ATA_REG_LBA1, BYTE(lba, 1));
		ide_write(channel, ATA_REG_LBA2, BYTE(lba, 2));
	} else {
		// Select drive, and any remaining LBA bits in LBA28 mode
		if (slave) {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xf0 | BYTE(lba, 3));
		} else {
			ide_write(channel, ATA_REG_HDDEVSEL, 0xe0 | BYTE(lba, 3));
		}

		ide_write(channel, ATA_REG_SECCOUNT0, BYTE(count, 0));
		ide_write(channel, ATA_REG_LBA0, BYTE(lba, 0));
		ide_write(channel, ATA_REG_LBA1, BYTE(lba, 1));
		ide_write(channel, ATA_REG_LBA2, BYTE(lba, 2));
	}
}

static void ide_delay400(idechannel_t * channel)
{
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
	ide_read(channel, ATA_REG_ALTSTATUS);
}

static void ide_check_status(idechannel_t * channel, uint8_t status)
{
	if (status & ATA_SR_ERR) {
		uint8_t error = ide_read(channel, ATA_REG_ERROR);
		KTHROWF(Exception, "%d", error);
	}
}

static uint8_t ide_wait(idechannel_t * channel, int polling)
{
	uint8_t status = 0;
#if 1
	do {
		status = ide_read(channel, ATA_REG_ALTSTATUS);
		if (status & 0x80) {
			if (polling) {
				thread_yield();
			} else {
				interrupt_monitor_wait_timeout(channel->lock, 1000000);
			}
		}
	} while(status & 0x80);
	status = ide_read(channel, ATA_REG_STATUS);
#else
	if ( 1 || channel->polling) {
		do {
			int sleeptime=100;
			status = ide_read(channel, ATA_REG_STATUS);
			if (status & 0x80) {
				timer_sleep(sleeptime);
				if (sleeptime<1000) {
					sleeptime += (sleeptime/2);
				}
			}
		} while(status & 0x80);
	} else {
	}
#endif
	ide_check_status(channel, status);

	return status;
}

static void ide_command(idechannel_t * channel, uint8_t command)
{
	ide_write(channel, ATA_REG_CONTROL, channel->polling ? 2 : 0);
	ide_delay400(channel);
	ide_write(channel, ATA_REG_COMMAND, command);
}

static void ide_drive_identify(idechannel_t * channel, int slave, uint8_t * buf, size_t bufsize)
{
	ide_address(channel, slave, 0, 0);
	ide_command(channel, ATA_CMD_IDENTIFY);
	assert(IDE_SECTORSIZE<=bufsize);
	ide_wait(channel, channel->polling);
	ide_read_pio(channel, buf, bufsize);
}

static void ide_drive_transfer_sectors(idechannel_t * channel, int slave, off64_t lba, int write, void * buf, size_t bufsize)
{
	size_t count = bufsize >> IDE_SECTORSIZE_LOG2;
	char * p = buf;

	check_int_bounds(count, 1, 256, "Sector count out of bounds");
	ide_address(channel, slave, lba, count);
	if (lba < 1<<28) {
		if (write) {
			ide_command(channel, ATA_CMD_WRITE_PIO);
		} else {
			ide_command(channel, ATA_CMD_READ_PIO);
		}
	} else {
		if (write) {
			ide_command(channel, ATA_CMD_WRITE_PIO_EXT);
		} else {
			ide_command(channel, ATA_CMD_READ_PIO_EXT);
		}
	}
	for(int i=0; i<count; i++) {
		ide_wait(channel, channel->polling);

		if (write) {
			ide_write_pio(channel, p, IDE_SECTORSIZE);
		} else {
			ide_read_pio(channel, p, IDE_SECTORSIZE);
		}
		p += IDE_SECTORSIZE;
	}
	if (write) {
		ide_command(channel, ATA_CMD_CACHE_FLUSH);
		ide_wait(channel, 1);
	}
}

static void ide_probe(uint8_t bus, uint8_t slot, uint8_t function)
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

		ide_initialize(bar0, bar1, bar2, bar3, bar4, irq);
	}
}

void ide_pciscan()
{
	pci_scan_class(ide_probe, 1, 1, 0x80, 0xffff, 0xffff);
}
