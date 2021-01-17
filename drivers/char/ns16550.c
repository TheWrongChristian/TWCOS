#include "ns16550.h"

/*
 * NS16550 compatible UART - Assumes ISA I/O ports, accessible via isa_port* functions
 * Platforms where the UART is memory mapped would handle this in their isa_port
 * implementations.
 */

typedef struct ns16550_device {
	interrupt_monitor_t * lock;
	vnode_t vnode[1];

	fifo_t * inq;
	fifo_t * outq;

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
	uint8_t data = fifo_get(dev->outq);
	isa_outb(dev->baseport, data);
}

static void ns16550_int(void * p)
{
	ns16550_device_t * dev = p;

	INTERRUPT_MONITOR_AUTOLOCK(dev->lock) {
		int iir=isa_inb(dev->baseport+2);
		if (0 == (iir & 1)) {
			switch(iir >> 1) {
			case 0:
				isa_inb(dev->baseport+6);
				break;
			case 1:
				if (!fifo_empty(dev->outq)) {
					ns16550_outq(dev);
				}
				break;
			case 2:
				if (!fifo_full(dev->inq)) {
					fifo_put(dev->inq, isa_inb(dev->baseport));
				} else {
					/* Discard */
					isa_inb(dev->baseport);
				}
				break;
			case 3:
				isa_inb(dev->baseport+6);
				break;
			case 6:
				isa_inb(dev->baseport);
				break;
			}
		}
	}
}

static size_t ns16550_read(vnode_t * vnode, off64_t ignored, void * buf, size_t len)
{
	ns16550_device_t * dev = container_of(vnode, ns16550_device_t, vnode);

	INTERRUPT_MONITOR_AUTOLOCK(dev->lock) {
		char * p = buf;
		for(int i=0; i<len; i++) {
			p[i] = fifo_get(dev->inq);
		}
	}

	return len;
}

static size_t ns16550_write(vnode_t * vnode, off64_t ignored, void * buf, size_t len)
{
	ns16550_device_t * dev = container_of(vnode, ns16550_device_t, vnode);

	INTERRUPT_MONITOR_AUTOLOCK(dev->lock) {
		char * p = buf;
		for(int i=0; i<len; i++) {
			if (fifo_full(dev->outq)) {
				ns16550_outq(dev);
			}
			fifo_put(dev->outq, p[i]);
		}
		ns16550_outq(dev);
	}

	return len;
}

vnode_t * ns16550_open(int baseport, int irq)
{
	ns16550_device_t * dev = 0;

	static vnode_ops_t ops = { .read = ns16550_read, .write = ns16550_write };
	static fs_t fs = { &ops };
	dev = calloc(1, sizeof(*dev));
	dev->lock = interrupt_monitor_irq(irq);
	dev->baseport = baseport;
	dev->inq = fifo_new(16);
	dev->outq = fifo_new(16);
	dev->divisor = 3;
	dev->line = 3;
	//isa_outb(dev->baseport+2, 0xC7);
	isa_outb(dev->baseport+2, 0);
	ns16550_control(dev);

	INTERRUPT_MONITOR_AUTOLOCK(dev->lock) {
		// Enable interrupts
		vnode_init(dev->vnode, VNODE_DEV, &fs);
		intr_add(irq, ns16550_int, dev);
		isa_outb(dev->baseport+1, 0xf);
	}

	return dev->vnode;
}

void ns16550_init()
{
        static GCROOT vnode_t * devfs = 0;

        if (0==devfs) {
                devfs = devfs_open();
        }

	static char * devfs_uart_base = "char/uart";
	static struct ns16550 {
		int port;
		int irq;
	} devices[] = {{0x3F8, 4}, {0x2f8, 3}};
	vnode_t * dir = vnode_newdir_hierarchy(devfs, devfs_uart_base);
	for(int i=0; i<countof(devices); i++) {
		char path[256];
		snprintf(path, countof(path), "%d", devices[i].port);
		vnode_put_vnode(dir, path, ns16550_open(devices[i].port, devices[i].irq));
	}
}
