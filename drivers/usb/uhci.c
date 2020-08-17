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
#define UHCI_PORTSC(x)	(0x10+x<<1)

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
#define UHCI_PORTSTS_RESUME	(1<<6)
#define UHCI_PORTSTS_LINESTATUS(status)	((status >> 4) & 0x3)
#define UHCI_PORTSTS_PEDC	(1<<3)
#define UHCI_PORTSTS_PED	(1<<2)
#define UHCI_PORTSTS_CSC	(1<<1)
#define UHCI_PORTSTS_CS		(1<<0)

typedef struct uhci_hcd_t uhci_hcd_t;
struct uhci_hcd_t {
	hcd_t hcd;
	int iobase;
	int irq;
	vmpage_t * pframelist;
	uintptr_t * framelist;
};

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
	hcd->irq = irq;
	hcd->pframelist = vmpage_calloc(CORE_SUB4G);
	hcd->framelist = vm_kas_get_aligned(ARCH_PAGE_SIZE, ARCH_PAGE_SIZE);
	vmap_map(kas, hcd->framelist, hcd->pframelist->page, 0, 0);

	/* Physical frame address */
	isa_outl(hcd->iobase+UHCI_FRBASEADD, hcd->pframelist->page << ARCH_PAGE_SIZE_LOG2);

	/* Enable all interrupts */
	isa_outw(hcd->iobase+UHCI_USBINTR, UHCI_USBINT_ALL);

	isa_outw(hcd->iobase+UHCI_FRNUM, 0);
	isa_outw(hcd->iobase+UHCI_SOFMOD, 0x40);

	/* Clear all status */
	isa_outw(hcd->iobase+UHCI_USBSTS, 0xffff);

	/* Start the schedule */
	isa_outw(hcd->iobase+UHCI_USBCMD, UHCI_USBCMD_CF | UHCI_USBCMD_RS);

	return &hcd->hcd;
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
