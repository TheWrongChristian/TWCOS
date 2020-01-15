#include "ns16550.h"

/*
 * NS16550 compatible UART - Assumes ISA I/O ports, accessible via isa_port* functions
 * Platforms where the UART is memory mapped would handle this in their isa_port
 * implementations.
 */

typedef struct ns16550_device {
	vnode_t vnode[1];

	queue_t * inq;
	queue_t * outq;

	int baseport;
	int divisor;
	int line;
} ns16550_device_t;

static void ns16550_control(ns16550_device_t * dev)
{
	uint8_t a = isa_inb(dev->baseport + 3);
	a |= 128; /* DLAB bit */
	isa_outb(dev->baseport+3, a);

	/* Baud rate divisor */
	isa_outb(dev->baseport, dev->divisor & 0xff);
	isa_outb(dev->baseport+1, (dev->divisor >> 8) & 0xff);

	/* Set line settings (num bits, parity etc.) */
	isa_outb(dev->baseport+3, dev->line);
}

static void ns16550_outq(ns16550_device_t * dev)
{
	uint8_t data = queue_get(dev->outq);
	isa_outb(dev->baseport, data);
}

static int devices_lock[1]={0};
static GCROOT map_t * devices = 0;

static void ns16550_int(int irq)
{
	ns16550_device_t * dev;

	SPIN_AUTOLOCK(devices_lock) {
		dev = map_getip(devices, irq);
	}

	if (0==dev) {
		/* No device on this IRQ */
		return;
	}

	int iir=isa_inb(dev->baseport+2);
	if (iir & 1) {
		/* No interrupt pending */
		return;
	}

	switch(iir>>1) {
	case 1:
		if (!queue_empty(dev->outq)) {
			ns16550_outq(dev);
		}
		break;
	case 2:
		queue_put(dev->inq, isa_inb(dev->baseport));
		break;
	}
#if 0
	uint8_t status = isa_inb(dev->baseport+5);

	if (status & 0x1) {
		KTRY {
			uint8_t data = isa_inb(dev->baseport);
			queue_put(dev->inq, data);
		} KCATCH(Exception) {
			// FIXME: Do error something here
		}
	}
	if (status & 0x1<<5) {
		if (!queue_empty(dev->outq)) {
			KTRY {
				ns16550_outq(dev);
			} KCATCH(Exception) {
				// FIXME: Do error something here
			}
		}
	}
#endif
}

static size_t ns16550_read(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	ns16550_device_t * dev = container_of(vnode, ns16550_device_t, vnode);
	char * p = buf;
	for(int i=0; i<len; i++) {
		p[i] = queue_get(dev->inq);
	}

	return len;
}

static size_t ns16550_write(vnode_t * vnode, off_t ignored, void * buf, size_t len)
{
	ns16550_device_t * dev = container_of(vnode, ns16550_device_t, vnode);
	char * p = buf;
	for(int i=0; i<len; i++) {
		if (queue_full(dev->outq)) {
			ns16550_outq(dev);
		}
		queue_put(dev->outq, p[i]);
	}
	ns16550_outq(dev);

	return len;
}

vnode_t * ns16550_open(int baseport, int irq)
{
	ns16550_device_t * dev = 0;

	SPIN_AUTOLOCK(devices_lock) {
		if (0==devices) {
			devices = splay_new(0);
		}

		dev = map_getip(devices, irq);
		if (0 == dev) {
			static vfs_ops_t ops = { read: ns16550_read, write: ns16550_write };
			static fs_t fs = { &ops };
			dev = calloc(1, sizeof(*dev));
			dev->baseport = baseport;
			dev->inq = queue_new(16);
			dev->outq = queue_new(16);
			dev->divisor = 3;
			dev->line = 3;
			//isa_outb(dev->baseport+2, 0xC7);
			isa_outb(dev->baseport+2, 0);
			ns16550_control(dev);
			map_putip(devices, irq, dev);

			// Enable interrupts
			add_irq(irq, ns16550_int);
			isa_outb(dev->baseport+1, 0xf);
			spin_unlock(devices_lock);
			ns16550_int(irq);
			spin_lock(devices_lock);

			vnode_init(dev->vnode, VNODE_DEV, &fs);
		}
	}

	if (dev) {
		return dev->vnode;
	}

	return 0;
}
