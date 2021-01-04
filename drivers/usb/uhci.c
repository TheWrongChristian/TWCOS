#include <stdint.h>
#include <sys/types.h>
#include "uhci.h"

#if INTERFACE

#endif

#define UHCI_USBCMD	0x0
#define UHCI_USBSTS	0x2
#define UHCI_USBINTR	0x4
#define UHCI_FRNUM	0x6
#define UHCI_FRBASEADD	0x8
#define UHCI_SOFMOD	0xc
#define UHCI_PORTSC(x)	(0x10+(x<<1))

#define UHCI_USBCMD_MAXP	(1<<7)
#define UHCI_USBCMD_CF		(1<<6)
#define UHCI_USBCMD_SWDBG	(1<<5)
#define UHCI_USBCMD_FGR		(1<<4)
#define UHCI_USBCMD_EGSM	(1<<3)
#define UHCI_USBCMD_GRESET	(1<<2)
#define UHCI_USBCMD_HCRESET	(1<<1)
#define UHCI_USBCMD_RS		(1<<0)

#define UHCI_USBSTS_HALTED	(1<<5)
#define UHCI_USBSTS_HCPE	(1<<4)
#define UHCI_USBSTS_HSE		(1<<3)
#define UHCI_USBSTS_RESUMEDETECT	(1<<2)
#define UHCI_USBSTS_ERRORINT	(1<<1)
#define UHCI_USBSTS_INT		(1<<0)

#define UHCI_USBINT_SPIE	(1<<3)
#define UHCI_USBINT_IOCE	(1<<2)
#define UHCI_USBINT_RIE		(1<<1)
#define UHCI_USBINT_TCRCIE	(1<<0)
#define UHCI_USBINT_ALL		(0xf)

#define UHCI_PORTSTS_SUSPEND	(1<<12)
#define UHCI_PORTSTS_RESET	(1<<9)
#define UHCI_PORTSTS_LSD	(1<<8)
#define UHCI_PORTSTS_PORT	(1<<7)
#define UHCI_PORTSTS_RESUME	(1<<6)
#define UHCI_PORTSTS_LINESTATUS(status)	((status >> 4) & 0x3)
#define UHCI_PORTSTS_PEDC	(1<<3)
#define UHCI_PORTSTS_PED	(1<<2)
#define UHCI_PORTSTS_CSC	(1<<1)
#define UHCI_PORTSTS_CS		(1<<0)

typedef union tdqlink tdqlink;
typedef struct uhci_td uhci_td;
typedef struct uhci_q uhci_q;
typedef struct uhci_hcd_t uhci_hcd_t;

union tdqlink {
	uhci_q * q;
        uhci_td * td;
};

struct uhci_q {
	/* Hardware fields */
	le32_t headlink;
	le32_t elementlink;

	/* Software fields */
	uhci_hcd_t * hcd;
	tdqlink vheadlink;
	tdqlink velementlink;
	future_t * future;
};

struct uhci_td {
	/* Hardware fields */
	le32_t link;
	le32_t flags;
	le32_t address;
	le32_t p;

	/* Software fields */
	uhci_hcd_t * hcd;
	tdqlink vlink;
	void * vp;
};

enum uhci_queue { 
	bulkq, controlq,
	periodicq1, periodicq2, periodicq4, periodicq8,
	periodicq16, periodicq32, periodicq64, periodicq128,
	maxq
};

struct uhci_hcd_t {
	hcd_t hcd;
	int iobase;
	interrupt_monitor_t * lock;
	thread_t * thread;
	vmpage_t * pframelist;
	uintptr_t * framelist;
	int ports;
	uhci_q * queues[maxq];
	map_t * pending;

	/* Interrupt information */
	uint16_t status;
};

static void uhci_status_check(uhci_q * q, future_t * future)
{
	uhci_td * td = q->velementlink.td;

	while(td) {
		int status = bitget(le32(td->flags), 23, 8);
		if (0x80 & status) {
			/* Still active - ignore */
			return;
		} else if (status) {
			/* Check for errors */
			future_set(future, status);
			return;
		}
		td = td->vlink.td;
	}

	/* By here, we've checked all the status as successful. */
	future_set(future, 0);

	/* FIXME: cleanup pending */
}

static void uhci_walk_pending(void * p, void * key, void * data)
{
#if 0
	uhci_hcd_t * hcd = p;
#endif
	uhci_q * q = key;
	future_t * future = data;
	uhci_status_check(q, future);
}

static void uhci_async_processor(uhci_hcd_t * hcd)
{
	while(1) {
		INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
			while(0 == hcd->status) {
				interrupt_monitor_wait(hcd->lock);
			}

			/* FIXME: Handle any errors */

			/* Process any pending frames */
			map_walkpp(hcd->pending, uhci_walk_pending, hcd);
			hcd->status = 0;
		}
#if 0
		timer_sleep(100000);
#endif
	}
}

static void uhci_irq(void * p)
{
	uhci_hcd_t * hcd = p;
	INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
#if 0
		uint16_t frame;
		uint16_t status;
		uint16_t command;
		frame = isa_inw(hcd->iobase + UHCI_FRNUM);
#endif
		hcd->status = isa_inw(hcd->iobase + UHCI_USBSTS);
		if (hcd->status & 0xf) {
			isa_outw(hcd->iobase + UHCI_USBSTS, 0xf);
			interrupt_monitor_broadcast(hcd->lock);
		}
	}
}

static void uhci_count_ports(uhci_hcd_t * hcd)
{
	for(int i=0; i<8; i++) {
		uint16_t status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
		if (UHCI_PORTSTS_PORT & status) {
			isa_outw(hcd->iobase+UHCI_PORTSC(i), (status & ~UHCI_PORTSTS_PORT));
			status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
			if (0==(status & UHCI_PORTSTS_PORT)) {
				break;
			}
			status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
			isa_outw(hcd->iobase+UHCI_PORTSC(i), (status | UHCI_PORTSTS_PORT));
			status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
			if (0==(status & UHCI_PORTSTS_PORT)) {
				break;
			}
			isa_outw(hcd->iobase+UHCI_PORTSC(i), (status | 0xa));
			status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
			if (status & 0xa) {
				break;
			}
			hcd->ports = i+1;
		} else {
			break;
		}
	}
}

static void uhci_reset_ports(uhci_hcd_t * hcd)
{
	for(int i=0; i<hcd->ports; i++) {
		uint16_t status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
		status |= UHCI_PORTSTS_RESET;
		isa_outw(hcd->iobase+UHCI_PORTSC(i), status);
		timer_sleep(50000);
		status &= (~UHCI_PORTSTS_RESET);
		isa_outw(hcd->iobase+UHCI_PORTSC(i), status);
		timer_sleep(10000);

		for(int j=0; j<16; j++) {
			status = isa_inw(hcd->iobase+UHCI_PORTSC(i));
			if (0==(UHCI_PORTSTS_CS & status)) {
				/* Nothing attached */
				break;
			}
			if (status & (UHCI_PORTSTS_PEDC | UHCI_PORTSTS_CSC)) {
				status &= ~(UHCI_PORTSTS_PEDC | UHCI_PORTSTS_CSC);
				isa_outw(hcd->iobase+UHCI_PORTSC(i), status);
				continue;
			}

			if (status & UHCI_PORTSTS_PED) {
				break;
			}

			status |= UHCI_PORTSTS_PED;
			isa_outw(hcd->iobase+UHCI_PORTSC(i), status);
		}
	}
}

typedef union cache_entry cache_entry;
union cache_entry {
	uhci_td td;
	uhci_q q;
	union cache_entry * next;
};

static spin_t cachelock[1];
static cache_entry * cache = 0;

static void uhci_entry_put(cache_entry * entry)
{
	/* cachelock is required here */
	entry->next = cache;
	cache = entry;
}

static cache_entry * uhci_entry_get()
{
	static arena_t * arena = 0;
	cache_entry * entry = 0;

	SPIN_AUTOLOCK(cachelock) {
		if (0==arena) {
			arena = arena_get();
		}

		if (0==cache) {
			char * entries = arena_palloc(arena, 1);
			vmpage_t * page = vm_page_get(entries);
			vmpage_setflags(page, VMPAGE_PINNED);
			size_t entrysize = ROUNDUP(sizeof(cache_entry), 0x10);
			for(int i = 0; i<ARCH_PAGE_SIZE/entrysize; i++) {
				uhci_entry_put((cache_entry *)(entries+(entrysize*i)));
			}
		}
		entry = cache;
		cache = cache->next;
	}

	return entry;
}

static void uhci_set_link(le32_t * plink, tdqlink * vlink, uhci_q * q, uhci_td * td, int depth)
{
	if (q) {
		*plink = le32(uhci_pa(q) | 0x2 );
		if (vlink) {
			vlink->q = q;
		}
	} else if (td) {
		*plink = le32(uhci_pa(td) | (depth ? 0x4 : 0));
		if (vlink) {
			vlink->td = td;
		}
	} else {
		*plink = le32(1);
		if (vlink) {
			vlink->td = 0;
		}
	}
}

static void uhci_q_headlink(uhci_q * q, uhci_q * lq, uhci_td * ltd)
{
	uhci_set_link(&q->headlink, &q->vheadlink, lq, ltd, 0);
}

static void uhci_q_elementlink(uhci_q * q, uhci_q * lq, uhci_td * ltd)
{
	uhci_set_link(&q->elementlink, &q->velementlink, lq, ltd, 0);
}

static void uhci_td_link(uhci_td * td, uhci_q * lq, uhci_td * ltd)
{
	uhci_set_link(&td->link, &td->vlink, lq, ltd, 1);
}

static uhci_q * uhci_q_get()
{
	uhci_q * q = &uhci_entry_get()->q;

	uhci_q_headlink(q, 0, 0);
	uhci_q_elementlink(q, 0, 0);

	return q;
}

static uhci_td * uhci_td_get()
{
	uhci_td * td = &uhci_entry_get()->td;

	uhci_td_link(td, 0, 0);
	td->flags = 0;
	td->address = 0;
	td->p = 0;
	td->vp = 0;

	return td;
}

static void uhci_q_free(uhci_q * req)
{
	SPIN_AUTOLOCK(cachelock) {
		uhci_td * td = req->velementlink.td;
		cache_entry * entry = (cache_entry*)req;
		uhci_entry_put(entry);
		while(td) {
			entry = (cache_entry*)td;
			td = td->vlink.td;
			uhci_entry_put(entry);
		}
	}
}

uint32_t uhci_pa(void * p)
{
	return ((vmap_get_page(kas, p) << ARCH_PAGE_SIZE_LOG2) | ARCH_PTRI_OFFSET(p));
}

void uhci_submit_request(urb_t * urb)
{
	uhci_hcd_t * hcd = container_of(urb->hcd, uhci_hcd_t, hcd);

	INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
	}
}

static future_t * uhci_packet(hcd_t * hcd, usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen);

hcd_t * uhci_reset(int iobase, int irq)
{
	/* Global reset 5 times with 10ms each */
	for(int i=0; i<5; i++) {
		isa_outw(iobase+UHCI_USBCMD, UHCI_USBCMD_GRESET);
		timer_sleep(10500);
		isa_outw(iobase+UHCI_USBCMD, 0);
	}
	timer_sleep(50000);

	if (0 != isa_inw(iobase+UHCI_USBCMD)) {
		return 0;
	}
	if (UHCI_USBSTS_HALTED != isa_inw(iobase+UHCI_USBSTS)) {
		return 0;
	}

	isa_outw(iobase+UHCI_USBSTS, 0x00ff);

	if (isa_inw(iobase+UHCI_SOFMOD) != 0x40) {
		return 0;
	}

	isa_outw(iobase+UHCI_USBCMD, UHCI_USBCMD_HCRESET);
	timer_sleep(42000);
	if (UHCI_USBCMD_HCRESET==isa_inw(iobase+UHCI_USBCMD)) {
		return 0;
	}

	uhci_hcd_t * hcd = calloc(1, sizeof(*hcd));
	hcd->iobase = iobase;
	hcd->lock = interrupt_monitor_irq(irq);
	hcd->pframelist = vmpage_calloc(CORE_SUB4G);
	hcd->framelist = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
	hcd->pending = arraymap_new(0, 64);

	static hcd_ops_t ops = { .packet = uhci_packet };	
	hcd->hcd.ops = &ops;
	vmpage_map(hcd->pframelist, kas, hcd->framelist, 1, 0);

	for(int i=0; i<countof(hcd->queues); i++) {
		hcd->queues[i] = uhci_q_get();
		if (i) {
			uhci_q_headlink(hcd->queues[i], hcd->queues[i-1], 0);
		}
	}

	/* Initialize schedule */
	for(int i=0; i<1024; i++) {
		if (0==i%128) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq128]) | 0x2);
		} else if (0==i%64) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq64]) | 0x2);
		} else if (0==i%32) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq32]) | 0x2);
		} else if (0==i%16) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq16]) | 0x2);
		} else if (0==i%8) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq8]) | 0x2);
		} else if (0==i%4) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq4]) | 0x2);
		} else if (0==i%2) {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq2]) | 0x2);
		} else {
			hcd->framelist[i] = le32(uhci_pa(hcd->queues[periodicq1]) | 0x2);
		}
	}

	/* Physical frame address */
	isa_outl(hcd->iobase+UHCI_FRBASEADD, hcd->pframelist->page << ARCH_PAGE_SIZE_LOG2);

	/* Enable all interrupts */
	isa_outw(hcd->iobase+UHCI_USBINTR, UHCI_USBINT_ALL);

	isa_outw(hcd->iobase+UHCI_FRNUM, 0);
	isa_outw(hcd->iobase+UHCI_SOFMOD, 0x40);

	/* Clear all status */
	isa_outw(hcd->iobase+UHCI_USBSTS, 0xffff);

	/* Count the ports */
	uhci_count_ports(hcd);

	/* Reset the ports */
	uhci_reset_ports(hcd);

	/* Start the schedule */
	isa_outw(hcd->iobase+UHCI_USBCMD, UHCI_USBCMD_CF | UHCI_USBCMD_RS);

#if 1
	thread_t * thread = thread_fork();
	if (0==thread) {
		uhci_async_processor(hcd);
	} else {
		hcd->thread = thread;
	}
#endif

	intr_add(irq, uhci_irq, hcd);

	return &hcd->hcd;
}

static uhci_q * uhci_td_chain(usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen)
{
	char * cp = buf;
	if (buf) {
		/* Check for crossing non-contiguous page boundaries */
		if (uhci_pa(cp + buflen)-uhci_pa(buf) != buflen) {
			/* FIXME: exception */
			return NULL;
		}
	}

	int togglebit;
	switch(pid) {
	case usbin:
		togglebit = 1;
		break;
	case usbout:
		togglebit = 2;
		break;
	case usbsetup:
		togglebit = 3;
		break;
	default:
		kernel_panic("");
		break;
	}

	ssize_t maxlen = (endpoint->device->flags & USB_DEVICE_LOW_SPEED) ? 8 : 64;
	uhci_td * head = 0;
	uhci_td * tail = 0;
	do {
		uhci_td * td = uhci_td_get();

		uint32_t flags = bitset(0, 26, 1, (endpoint->device->flags & USB_DEVICE_LOW_SPEED) ? 1 : 0);
		flags = bitset(flags, 28, 2, 3);
		flags = bitset(flags, 24, 1, (buflen<=maxlen));
		flags = bitset(flags, 23, 8, 0x80);

		uint32_t address = bitset(0, 31, 11, (buflen>maxlen) ? maxlen-1 : buflen-1);
		address = bitset(address, 18, 4, endpoint->endp);
		address = bitset(address, 14, 7, endpoint->device->dev);
		address = bitset(address, 19, 1, (togglebit & endpoint->toggle) ? 1 : 0);
		endpoint->toggle ^= togglebit;

		switch(pid) {
		case usbsetup:
			address = bitset(address, 7, 8, 0x2d);
			break;
		case usbin:
			address = bitset(address, 7, 8, 0x69);
			break;
		case usbout:
			address = bitset(address, 7, 8, 0xe1);
			break;
		}

		td->flags = le32(flags);
		td->address = le32(address);
		if (buf) {
			td->p = le32(uhci_pa(cp));
			td->vp = cp;
		} else {
			td->p = 0;
		}
		
		if (0 == head) {
			head = td;
		} else {
			uhci_td_link(tail, 0, td);
		}
		tail = td;
		if (buflen) {
			if (buflen<maxlen) {
				buflen = 0;
			} else {
				buflen -= maxlen;
				cp += maxlen;
			}
		}
	} while(buflen>0);

	uhci_q * q = uhci_q_get();
	uhci_q_elementlink(q, 0, head);

	return q;
}

static void uhci_q_remove(uhci_hcd_t * hcd, uhci_q * req)
{
	for(int i=0; i<countof(hcd->queues); i++) {
		/* Handle queue head */
		if (req == hcd->queues[i]->velementlink.q) {
			uhci_q_elementlink(hcd->queues[i], req->vheadlink.q, 0);
			return;
		}

		/* Look for the request in the queue list */
		uhci_q * prev = hcd->queues[i];
		while(prev) {
			if(req == prev->vheadlink.q) {
				/* unlink */
				uhci_q_headlink(prev, req->vheadlink.q, 0);
				return;
			}
			prev = prev->vheadlink.q;
		}
	}
}

static void uhci_cleanup_packet(void * p)
{
	uhci_q * req = p;
	assert(req->elementlink == le32(1));
	uhci_hcd_t * uhci_hcd = req->hcd;

	INTERRUPT_MONITOR_AUTOLOCK(uhci_hcd->lock) {
		uhci_q_remove(uhci_hcd, req);
		map_removepp(uhci_hcd->pending, req);
	}

	uhci_q_free(req);
}

static future_t * uhci_packet(hcd_t * hcd, usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen)
{
	uhci_q * req = uhci_td_chain(pid, endpoint, buf, buflen);
	future_t * future = future_create(uhci_cleanup_packet, req);
	enum uhci_queue queue;

	if (usbsetup == pid) {
		queue = controlq;
	} else if (endpoint->periodic) {
		for(queue=periodicq1; queue<maxq; queue++) {
			if ((1<<(queue-periodicq1))<endpoint->periodic) {
				queue--;
				break;
			}
		}
	} else {
		queue = bulkq;
	}

	uhci_hcd_t * uhci_hcd = container_of(hcd, uhci_hcd_t, hcd);
	INTERRUPT_MONITOR_AUTOLOCK(uhci_hcd->lock) {
		uhci_q * q = uhci_hcd->queues[queue];
		uhci_q * nextq = q->vheadlink.q;
		uhci_q_headlink(req, q->vheadlink.q, 0);

		uhci_q * tail = q->velementlink.q;
		while(tail) {
			uhci_q * tempq = tail;
			if (tail->vheadlink.q != nextq) {
				tail = tail->vheadlink.q;
			} else {
				tail = 0;
			}
			if (le32(1)==tempq->elementlink) {
				/* Queue has been processed */
				uhci_q_elementlink(q, tail, 0);
			}
		}
		if (tail) {
			uhci_q_headlink(tail, req, 0);
		} else {
			uhci_q_elementlink(q, req, 0);
		}

		req->hcd = uhci_hcd;
		map_putpp(uhci_hcd->pending, req, future);
	}

	return future;
}

void uhci_probe(uint8_t bus, uint8_t slot, uint8_t function)
{
	uintptr_t bar4 = pci_bar_base(bus, slot, function, 4);
	int irq = pci_irq(bus, slot, function);
	static GCROOT hcd_t * hcd;
	hcd = uhci_reset(bar4, irq);
	usb_test(hcd);
}

void uhci_pciscan()
{
	pci_scan_class(uhci_probe, 0xc, 0x3, 0, 0xffff, 0xffff);
}