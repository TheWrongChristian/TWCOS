#include <stdint.h>
#include <sys/types.h>
#include "ehci.h"

#if INTERFACE

#endif

#define EHCI_USBCMD	0x0
#define EHCI_USBSTS	0x1
#define EHCI_USBINTR	0x2
#define EHCI_FRNUM	0x3
#define EHCI_FRBASEADD	0x5
#define EHCI_ASYNCADD	0x6
#define EHCI_PORTSC(x)	(0x11+x)

#define EHCI_USBCMD_MAXP	(1<<7)
#define EHCI_USBCMD_ASYNCINT	(1<<6)
#define EHCI_USBCMD_ASYNC	(1<<5)
#define EHCI_USBCMD_PERIODIC	(1<<4)
#define EHCI_USBCMD_FRLISTSZ(s)	(s<<2)
#define EHCI_USBCMD_HCRESET	(1<<1)
#define EHCI_USBCMD_RS		(1<<0)

#define EHCI_USBSTS_HALTED	(1<<12)
#define EHCI_USBSTS_ASYNCADV	(1<<5)
#define EHCI_USBSTS_HOSTERR	(1<<4)
#define EHCI_USBSTS_FRROLLOVER	(1<<3)
#define EHCI_USBSTS_PORTCHANGE	(1<<2)
#define EHCI_USBSTS_ERRORINT	(1<<1)
#define EHCI_USBSTS_INT		(1<<0)

#define EHCI_USBINT_SPIE	(1<<3)
#define EHCI_USBINT_IOCE	(1<<2)
#define EHCI_USBINT_RIE		(1<<1)
#define EHCI_USBINT_TCRCIE	(1<<0)
#define EHCI_USBINT_ALL		(0x3f)

#define EHCI_PORTSTS_SUSPEND	(1<<12)
#define EHCI_PORTSTS_RESET	(1<<9)
#define EHCI_PORTSTS_LSD	(1<<8)
#define EHCI_PORTSTS_PORT	(1<<7)
#define EHCI_PORTSTS_RESUME	(1<<6)
#define EHCI_PORTSTS_LINESTATUS(status)	((status >> 4) & 0x3)
#define EHCI_PORTSTS_PEDC	(1<<3)
#define EHCI_PORTSTS_PED	(1<<2)
#define EHCI_PORTSTS_CSC	(1<<1)
#define EHCI_PORTSTS_CS		(1<<0)

typedef union tdqlink tdqlink;
typedef struct ehci_td ehci_td;
typedef struct ehci_q ehci_q;
typedef struct ehci_hcd_t ehci_hcd_t;

union tdqlink {
	ehci_q * q;
        ehci_td * td;
};

struct ehci_q {
	/* Hardware fields */
	le32_t headlink;
	le32_t elementlink;

	/* Software fields */
	ehci_hcd_t * hcd;
	tdqlink vheadlink;
	tdqlink velementlink;
	future_t * future;
};

struct ehci_td {
	/* Hardware fields */
	le32_t link;
	le32_t flags;
	le32_t address;
	le32_t p;

	/* Software fields */
	ehci_hcd_t * hcd;
	tdqlink vlink;
	void * vp;
};

enum ehci_queue { 
	bulkq, controlq,
	periodicq1, periodicq2, periodicq4, periodicq8,
	periodicq16, periodicq32, periodicq64, periodicq128,
	maxq
};

struct ehci_hcd_t {
	hcd_t hcd;
	usb_hub_t roothub;
	int nports;
	usb_device_t ** ports;
	void * base;

	/* Operational registers */
	volatile uint32_t * opreg;
	
	interrupt_monitor_t * lock;
	thread_t * thread;
	vmpage_t * pframelist;
	uintptr_t * framelist;
	ehci_q * queues[maxq];
	map_t * pending;

	/* Interrupt information */
	uint16_t status;
};

static packet_field_t ehci_capreg_fields[] = {
	PACKET_FIELD(1),
	PACKET_FIELD(1),
	PACKET_FIELD(2),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(8),
};
static packet_def_t ehci_capreg[] = {PACKET_DEF(ehci_capreg_fields)};

static packet_field_t ehci_opreg_fields[] = {
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
	PACKET_FIELD(4),
};
static packet_def_t ehci_opreg[] = {PACKET_DEF(ehci_opreg_fields)};

static void ehci_status_check(ehci_q * q, future_t * future)
{
	ehci_td * td = q->velementlink.td;

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

static void ehci_walk_pending(const void * const p, void * key, void * data)
{
#if 0
	ehci_hcd_t * hcd = p;
#endif
	ehci_q * q = key;
	future_t * future = data;
	ehci_status_check(q, future);
}

static void ehci_async_processor(ehci_hcd_t * hcd)
{
	while(1) {
		KTRY {
			INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
				while(0 == hcd->status) {
	#if 0
					TRACE();
					interrupt_monitor_wait_timeout(hcd->lock, 10000000);
	#else
					interrupt_monitor_wait(hcd->lock);
	#endif
				}

				/* Process any pending frames */
				TRACE();
				map_walkpp(hcd->pending, ehci_walk_pending, hcd);
				hcd->status = 0;
			}
		} KCATCH(TimeoutException) {
			INTERRUPT_MONITOR_AUTOLOCK(hcd->lock) {
				map_walkpp(hcd->pending, ehci_walk_pending, hcd);
			}
		}
	}
}

static void ehci_irq(void * p)
{
	ehci_hcd_t * hcd = p;
	hcd->status = hcd->opreg[EHCI_USBSTS];
	if (hcd->status & 0x3f) {
		hcd->opreg[EHCI_USBSTS] = hcd->status & 0x3f;
		interrupt_monitor_broadcast(hcd->lock);
	}
}

static void ehci_reset_port(ehci_hcd_t * hcd, int port)
{
	uint32_t status = hcd->opreg[EHCI_PORTSC(port)];
}

typedef union cache_entry cache_entry;
union cache_entry {
	ehci_td td;
	ehci_q q;
	union cache_entry * next;
};

static spin_t cachelock[1];
static cache_entry * cache = 0;

static void ehci_entry_put(cache_entry * entry)
{
	/* cachelock is required here */
	entry->next = cache;
	cache = entry;
}

static cache_entry * ehci_entry_get()
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
				ehci_entry_put((cache_entry *)(entries+(entrysize*i)));
			}
		}
		entry = cache;
		cache = cache->next;
	}

	return entry;
}

static void ehci_set_link(le32_t * plink, tdqlink * vlink, ehci_q * q, ehci_td * td, int depth)
{
	if (q) {
		*plink = le32(ehci_pa(q) | 0x2 );
		if (vlink) {
			vlink->q = q;
		}
	} else if (td) {
		*plink = le32(ehci_pa(td) | (depth ? 0x4 : 0));
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

static void ehci_q_headlink(ehci_q * q, ehci_q * lq, ehci_td * ltd)
{
	ehci_set_link(&q->headlink, &q->vheadlink, lq, ltd, 0);
}

static void ehci_q_elementlink(ehci_q * q, ehci_q * lq, ehci_td * ltd)
{
	ehci_set_link(&q->elementlink, &q->velementlink, lq, ltd, 0);
}

static void ehci_td_link(ehci_td * td, ehci_q * lq, ehci_td * ltd)
{
	ehci_set_link(&td->link, &td->vlink, lq, ltd, 1);
}

static ehci_q * ehci_q_get()
{
	ehci_q * q = &ehci_entry_get()->q;

	ehci_q_headlink(q, 0, 0);
	ehci_q_elementlink(q, 0, 0);

	return q;
}

static ehci_td * ehci_td_get()
{
	ehci_td * td = &ehci_entry_get()->td;

	ehci_td_link(td, 0, 0);
	td->flags = 0;
	td->address = 0;
	td->p = 0;
	td->vp = 0;

	return td;
}

static void ehci_q_free(ehci_q * req)
{
	SPIN_AUTOLOCK(cachelock) {
		ehci_td * td = req->velementlink.td;
		cache_entry * entry = (cache_entry*)req;
		ehci_entry_put(entry);
		while(td) {
			entry = (cache_entry*)td;
			td = td->vlink.td;
			ehci_entry_put(entry);
		}
	}
}

uint32_t ehci_pa(void * p)
{
	return ((vmap_get_page(kas, p) << ARCH_PAGE_SIZE_LOG2) | ARCH_PTRI_OFFSET(p));
}

static future_t * ehci_submit(hcd_t * hcd, usb_request_t * request)
{
	return 0;
}

/* EHCI root hub */
static int ehci_hub_port_count(usb_hub_t * hub)
{
	ehci_hcd_t * hcd = container_of(hub, ehci_hcd_t, roothub);

	return hcd->nports;
}

static void ehci_hub_reset_port(usb_hub_t * hub, int port)
{
	ehci_hcd_t * hcd = container_of(hub, ehci_hcd_t, roothub);

	/* Reset the port */
	ehci_reset_port(hcd, port);
}

static usb_device_t * ehci_hub_get_device(usb_hub_t * hub, int port)
{
	usb_device_t * device = hub->ports[port];
	if (0 == device) {
		ehci_hub_reset_port(hub, port);
		device = hub->ports[port];
	}

	return device;
}

static void ehci_hub_disable_port(usb_hub_t * hub, int port)
{
	ehci_hcd_t * hcd = container_of(hub, ehci_hcd_t, roothub);
}

static volatile uint8_t * ehci_reg8(ehci_hcd_t * hcd, int offset)
{
	return ((uint8_t *)hcd->base) + offset;
}

static volatile uint16_t * ehci_reg16(ehci_hcd_t * hcd, int offset)
{
	return ((uint16_t *)hcd->base) + offset/sizeof(uint16_t);
}

static volatile uint32_t * ehci_reg32(ehci_hcd_t * hcd, int offset)
{
	return ((uint32_t *)hcd->base) + offset/sizeof(uint32_t);
}

static volatile uint64_t * ehci_reg64(ehci_hcd_t * hcd, int offset)
{
	return ((uint64_t *)hcd->base) + offset/sizeof(uint64_t);
}

static void ehci_stop(ehci_hcd_t * hcd)
{
	if (hcd->opreg[EHCI_USBCMD] & 1) {
		hcd->opreg[EHCI_USBCMD] &= ~1;
		while(0 == (hcd->opreg[EHCI_USBCMD] & EHCI_USBSTS_HALTED)) {
			timer_sleep(1000);
		}
	}
}

static void ehci_start(ehci_hcd_t * hcd)
{
	if (0 == (hcd->opreg[EHCI_USBCMD] & 1)) {
		hcd->opreg[EHCI_USBCMD] |= 1;
		while(hcd->opreg[EHCI_USBCMD] & EHCI_USBSTS_HALTED) {
			timer_sleep(1000);
		}
	}
}

static void ehci_dumpcap(ehci_hcd_t * hcd)
{
	kernel_printk("EHCI capreg: %p\n", hcd->base);
	kernel_printk("EHCI caplength: %d\n", packet_get(ehci_capreg, hcd->base, 0));
	kernel_printk("EHCI hciversion: %x\n", packet_get(ehci_capreg, hcd->base, 2));
	kernel_printk("EHCI hcsparams: %x\n", packet_get(ehci_capreg, hcd->base, 3));
	kernel_printk("EHCI hccparams: %x\n", packet_get(ehci_capreg, hcd->base, 4));
}

static void ehci_reset(ehci_hcd_t * hcd)
{
	int waittime = 1000; // 1000 ms max
	TRACE();
	ehci_stop(hcd);
	hcd->opreg[EHCI_USBCMD] |= EHCI_USBCMD_HCRESET;
	while(waittime-- && hcd->opreg[EHCI_USBCMD] & EHCI_USBCMD_HCRESET) {
		timer_sleep(1000);
	}

	if (hcd->opreg[EHCI_USBCMD] & EHCI_USBCMD_HCRESET) {
		KTHROWF(TimeoutException, "EHCI reset timed out: %p", hcd->base);
	}
}

static interface_map_t ehci_hcd_t_map [] =
{
	INTERFACE_MAP_ENTRY(ehci_hcd_t, iid_hcd_t, hcd),
	INTERFACE_MAP_ENTRY(ehci_hcd_t, iid_usb_hub_t, roothub),
};
static INTERFACE_IMPL_QUERY(hcd_t, ehci_hcd_t, hcd)
static INTERFACE_OPS_TYPE(hcd_t) INTERFACE_IMPL_NAME(hcd_t, ehci_hcd_t) = {
	INTERFACE_IMPL_QUERY_METHOD(hcd_t, ehci_hcd_t)
	INTERFACE_IMPL_METHOD(submit, ehci_submit)
};
static INTERFACE_IMPL_QUERY(usb_hub_t, ehci_hcd_t, hcd)
static INTERFACE_OPS_TYPE(usb_hub_t) INTERFACE_IMPL_NAME(usb_hub_t, ehci_hcd_t) = {
	INTERFACE_IMPL_QUERY_METHOD(usb_hub_t, ehci_hcd_t)
	INTERFACE_IMPL_METHOD(port_count, ehci_hub_port_count)
	INTERFACE_IMPL_METHOD(reset_port, ehci_hub_reset_port)
	INTERFACE_IMPL_METHOD(get_device, ehci_hub_get_device)
	INTERFACE_IMPL_METHOD(disable_port, ehci_hub_disable_port)
};

static ehci_hcd_t * ehci_init(void * base, int irq)
{
	ehci_hcd_t * hcd = calloc(1, sizeof(*hcd));
	hcd->base = base;
	ehci_dumpcap(hcd);
	uint32_t opreg_offset = packet_get(ehci_capreg, hcd->base, 0);
	hcd->opreg = (uint32_t*)((uint8_t*)hcd->base) + opreg_offset;
	hcd->lock = interrupt_monitor_irq(irq);
	hcd->pframelist = vmpage_calloc(CORE_SUB4G);
	hcd->framelist = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
	hcd->pending = arraymap_new(0, 64);

	hcd->hcd.ops = &ehci_hcd_t_hcd_t;
	bitarray_setall(hcd->hcd.ids, 32*countof(hcd->hcd.ids), 1);
	bitarray_set(hcd->hcd.ids, 0, 0);
	vmpage_map(hcd->pframelist, kas, hcd->framelist, 1, 0);


	ehci_reset(hcd);
#if 0
	for(int i=0; i<countof(hcd->queues); i++) {
		hcd->queues[i] = ehci_q_get();
		if (i) {
			ehci_q_headlink(hcd->queues[i], hcd->queues[i-1], 0);
		}
	}

	/* Initialize schedule */
	for(int i=0; i<1024; i++) {
		if (0==i%128) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq128]) | 0x2);
		} else if (0==i%64) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq64]) | 0x2);
		} else if (0==i%32) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq32]) | 0x2);
		} else if (0==i%16) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq16]) | 0x2);
		} else if (0==i%8) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq8]) | 0x2);
		} else if (0==i%4) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq4]) | 0x2);
		} else if (0==i%2) {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq2]) | 0x2);
		} else {
			hcd->framelist[i] = le32(ehci_pa(hcd->queues[periodicq1]) | 0x2);
		}
	}

	/* Physical frame address */
	isa_outl(hcd->iobase+EHCI_FRBASEADD, hcd->pframelist->page << ARCH_PAGE_SIZE_LOG2);

	/* Enable all interrupts */
	isa_outw(hcd->iobase+EHCI_USBINTR, EHCI_USBINT_ALL);

	isa_outw(hcd->iobase+EHCI_FRNUM, 0);
	isa_outw(hcd->iobase+EHCI_SOFMOD, 0x40);

	/* Clear all status */
	isa_outw(hcd->iobase+EHCI_USBSTS, 0xffff);

	/* Start the schedule */
	isa_outw(hcd->iobase+EHCI_USBCMD, EHCI_USBCMD_CF | EHCI_USBCMD_RS);
#endif
#if 1
	thread_t * thread = thread_fork();
	if (0==thread) {
		thread_set_name(0, "EHCI async processor");
		ehci_async_processor(hcd);
	} else {
		hcd->thread = thread;
	}
#endif
	hcd->roothub.ports = hcd->ports;
	hcd->roothub.ops = &ehci_hcd_t_usb_hub_t;

	intr_add(irq, ehci_irq, hcd);

	return hcd;
}

static ehci_q * ehci_td_chain(usbpid_t pid, usb_endpoint_t * endpoint, void * buf, size_t buflen)
{
#if 0
	char * cp = buf;
	if (buf) {
		/* Check for crossing non-contiguous page boundaries */
		if (ehci_pa(cp + buflen)-ehci_pa(buf) != buflen) {
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
	ehci_td * head = 0;
	ehci_td * tail = 0;
	do {
		ehci_td * td = ehci_td_get();

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
			td->p = le32(ehci_pa(cp));
			td->vp = cp;
		} else {
			td->p = 0;
		}
		
		if (0 == head) {
			head = td;
		} else {
			ehci_td_link(tail, 0, td);
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
#endif
	return 0;
}

static void ehci_q_remove(ehci_hcd_t * hcd, ehci_q * req)
{
	for(int i=0; i<countof(hcd->queues); i++) {
		/* Handle queue head */
		if (req == hcd->queues[i]->velementlink.q) {
			ehci_q_elementlink(hcd->queues[i], req->vheadlink.q, 0);
			return;
		}

		/* Look for the request in the queue list */
		ehci_q * prev = hcd->queues[i];
		while(prev) {
			if(req == prev->vheadlink.q) {
				/* unlink */
				ehci_q_headlink(prev, req->vheadlink.q, 0);
				return;
			}
			prev = prev->vheadlink.q;
		}
	}
}

static void ehci_cleanup_packet(void * p)
{
	ehci_q * req = p;
	assert(req->elementlink == le32(1));
	ehci_hcd_t * ehci_hcd = req->hcd;

	INTERRUPT_MONITOR_AUTOLOCK(ehci_hcd->lock) {
		ehci_q_remove(ehci_hcd, req);
		map_removepp(ehci_hcd->pending, req);
	}

	ehci_q_free(req);
}

static future_t * ehci_packet(hcd_t * hcd, usb_endpoint_t * endpoint, usbpid_t pid, void * buf, size_t buflen)
{
	ehci_q * req = ehci_td_chain(pid, endpoint, buf, buflen);
	future_t * future = future_create(ehci_cleanup_packet, req);
	enum ehci_queue queue;

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

	ehci_hcd_t * ehci_hcd = container_of(endpoint->device->hcd, ehci_hcd_t, hcd);
	INTERRUPT_MONITOR_AUTOLOCK(ehci_hcd->lock) {
		ehci_q * q = ehci_hcd->queues[queue];
		ehci_q * nextq = q->vheadlink.q;
		ehci_q_headlink(req, q->vheadlink.q, 0);

		TRACE();
		ehci_q * tail = q->velementlink.q;
		while(tail) {
			TRACE();
			ehci_q * tempq = tail;
			if (tail->vheadlink.q != nextq) {
				TRACE();
				tail = tail->vheadlink.q;
			} else {
				TRACE();
				tail = 0;
			}
			if (le32(1)==tempq->elementlink) {
				/* Queue has been processed */
				TRACE();
				ehci_q_elementlink(q, tail, 0);
			}
		}
		if (tail) {
			TRACE();
			ehci_q_headlink(tail, req, 0);
		} else {
			TRACE();
			ehci_q_elementlink(q, req, 0);
		}

		req->hcd = ehci_hcd;
		map_putpp(ehci_hcd->pending, req, future);
	}

	TRACE();
	return future;
}

void ehci_probe(device_t * device)
{
	TRACE();
	void * base = pci_bar_map(device, 0);
	int irq = pci_irq(device);
	static GCROOT ehci_hcd_t * hcd;
	TRACE();
	hcd = ehci_init(base, irq);
	if (hcd) {
		TRACE();
		usb_test(&hcd->roothub);
	}
}

void ehci_pciinit()
{
	device_driver_register(pci_progif_key(0xc, 0x3, 0x20), ehci_probe);
}
#if 0
STATIC_INIT staticinit_t ehci_pciinit_func = ehci_pciinit;
#endif
