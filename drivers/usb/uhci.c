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

typedef struct uhci_td uhci_td;
typedef struct uhci_q uhci_q;
struct uhci_q {
	/* Hardware fields */
	le32_t qlink;
	le32_t tdlink;

	/* Software fields */
	uhci_q * this;
	uhci_q * vqlink;
	uhci_td * vtdlink;
};

struct uhci_td {
	/* Hardware fields */
	le32_t link;
	le32_t flags;
	le32_t address;
	le32_t p;

	/* Software fields */
	uhci_td * this;
	void * vlink;
};

enum uhci_queues { 
	bulkq, controlq,
	periodicq1, periodicq2, periodicq4, periodicq8,
	periodicq16, periodicq32, periodicq64, periodicq128,
	maxq
};

typedef struct uhci_hcd_t uhci_hcd_t;
struct uhci_hcd_t {
	hcd_t hcd;
	int iobase;
	interrupt_monitor_t * lock;
	thread_t * thread;
	vmpage_t * pframelist;
	uintptr_t * framelist;
	int ports;
	uhci_q * queues[maxq];
};

static void uhci_interrupt_processor(uhci_hcd_t * hcd)
{
	INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
		while(hcd) {
			interrupt_monitor_wait(hcd->lock);
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
			cache_entry * entries = arena_palloc(arena, 1);
			vmpage_t * page = vm_page_get(entries);
			vmpage_setflags(page, VMPAGE_PINNED);
			for(int i = 0; i<ARCH_PAGE_SIZE/sizeof(*entries); i++) {
				uhci_entry_put(entries+i);
			}
		}
		entry = cache;
		cache = cache->next;
	}

	return entry;
}

static uhci_td * uhci_td_get()
{
	uhci_td * td = &uhci_entry_get()->td;
	td->link = 1;
	td->flags = 0;
	td->address = 0;
	td->p = 0;
	td->this = td;

	td->vlink = 0;

	return td;
}

static void uhci_td_link(uhci_td * td, void * p, int q)
{
	td->link = le32(uhci_pa(p) | (q) ? 0x2 : 0);
	td->vlink = p;
}

static uhci_q * uhci_q_get()
{
	uhci_q * q = &uhci_entry_get()->q;
	q->qlink = le32(1);
	q->tdlink = le32(1);
	q->this = q;

	q->vqlink = 0;
	q->vtdlink = 0;

	return q;
}

static void uhci_q_qlink(uhci_q * q, void * p, int isq)
{
	q->qlink = le32(uhci_pa(p) | (isq) ? 0x2 : 0);
	q->vqlink = p;
}

static void uhci_q_tdlink(uhci_q * q, void * p, int isq)
{
	q->tdlink = le32(uhci_pa(p) | (isq) ? 0x2 : 0);
	q->vtdlink = p;
}

static void uhci_entry_free(cache_entry * entry)
{
	SPIN_AUTOLOCK(cachelock) {
		uhci_entry_put(entry);
	}
}

uint32_t uhci_pa(void * p)
{
	return le32(vmap_get_page(kas, p) << ARCH_PAGE_SIZE_LOG2 | ARCH_PTRI_OFFSET(p));
}

void uhci_submit_request(urb_t * urb)
{
	uhci_hcd_t * hcd = container_of(urb->hcd, uhci_hcd_t, hcd);

	INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
	}
}

static void uhci_packet(hcd_t * hcd, usbpid_t pid, uint8_t dev, uint8_t endp, uint8_t ls, void * buf, size_t buflen);
hcd_t * uhci_reset(int iobase, int irq)
{
	/* Global reset 5 times with 10ms each */
	for(int i=0; i<5; i++) {
		isa_outw(iobase+UHCI_USBCMD, UHCI_USBCMD_GRESET);
		timer_sleep(10500);
		isa_outw(iobase+UHCI_USBCMD, 0);
	}

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

	static hcd_ops_t ops = { .packet = uhci_packet };	
	hcd->hcd.ops = &ops;
	vmap_map(kas, hcd->framelist, hcd->pframelist->page, 0, 0);

	for(int i=0; i<countof(hcd->queues); i++) {
		hcd->queues[i] = uhci_q_get();
		if (i) {
			hcd->queues[i]->qlink = le32(uhci_pa(hcd->queues[i-1]) | 0x2);
		} else {
			hcd->queues[i]->qlink = le32(1);
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

#if 0
	thread_t * thread = thread_fork();
	if (0==thread) {
		uhci_interrupt_processor(hcd);
	} else {
		hcd->thread = thread;
	}
#endif

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

	return &hcd->hcd;
}

static uhci_td * uhci_td_chain(usbpid_t pid, uint8_t dev, uint8_t endp, uint8_t ls, void * buf, size_t buflen)
{
	/* Check for crossing non-contiguous page boundaries */
	char * cp = buf;
	if (uhci_pa(cp + buflen)-uhci_pa(buf) != buflen) {
		/* FIXME: exception */
		return NULL;
	}

	size_t maxlen = (ls) ? 8 : 64;
	uhci_td * head = 0;
	uhci_td * tail = 0;
	do {
		uhci_td * td = uhci_td_get();

		uint32_t flags = bitset(0, 26, 1, ls);
		flags = bitset(flags, 23, 8, 0x80);

		uint32_t address = bitset(0, 31, 11, (buflen>maxlen) ? maxlen-1 : buflen-1);
		address = bitset(address, 18, 4, endp);
		address = bitset(address, 14, 7, dev);

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
		td->p = le32(uhci_pa(cp));
		
		if (0 == head) {
			head = td;
		} else {
			uhci_td_link(tail, td, 0);
		}
		tail = td;
	} while(buflen>0);

	return head;
}

static void uhci_packet(hcd_t * hcd, usbpid_t pid, uint8_t dev, uint8_t endp, uint8_t ls, void * buf, size_t buflen)
{
	uhci_td * td = uhci_td_chain(pid, dev, endp, ls, buf, buflen);
}

void uhci_probe(uint8_t bus, uint8_t slot, uint8_t function)
{
	uintptr_t bar4 = pci_bar_base(bus, slot, function, 4);
	int irq = pci_irq(bus, slot, function);
	hcd_t * hcd = uhci_reset(bar4, irq);
}

void uhci_pciscan()
{
	pci_scan_class(uhci_probe, 0xc, 0x3, 0, 0);
}
