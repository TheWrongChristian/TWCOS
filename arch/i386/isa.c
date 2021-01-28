#include <stdint.h>
#include <stdarg.h>

#include "isa.h"

#if INTERFACE

#define IRQMAX 16

#endif

/* PIT clock */
#define PIT_HZ		1193182

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define PIC1		0x20
#define PIC2		0xA0   
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI		0x20

#define PIC_IRQ_BASE	0x20
 
#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW3_READ_IRR	0x0a		/* OCW3 irq ready next CMD read */
#define ICW3_READ_ISR	0x0b		/* OCW3 irq service next CMD read */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */
 
/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
static void io_wait()
{
}

int irqmax = IRQMAX;

void PIC_eoi(int irq)
{
	if(irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);

	outb(PIC1_COMMAND,PIC_EOI);
}

void PIC_remap(int offset1, int offset2)
{
	unsigned char a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}

static irq_func irq_table[] =  {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

/* Helper func */
static uint16_t PIC_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

/* Returns the combined value of the cascaded PICs irq request register */
uint16_t PIC_get_irr(void)
{
    return PIC_get_irq_reg(ICW3_READ_IRR);
}

/* Returns the combined value of the cascaded PICs irq service register */
uint16_t PIC_get_isr(void)
{
    return PIC_get_irq_reg(ICW3_READ_ISR);
}

static void PIC_set_mask(int irq) {
	uint16_t port;
	uint8_t value;

	if(irq < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq -= 8;
	}
	value = inb(port) | (1 << irq);
	outb(port, value);        
}
 
static void PIC_clear_mask(int irq) {
	uint16_t port;
	uint8_t value;

	if(irq < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq -= 8;
	}
	value = inb(port) & ~(1 << irq);
	outb(port, value);        
}

static unsigned long spurious = 0;
int inirq = 0;
void i386_irq(uint32_t num, arch_trap_frame_t * state)
{
	int irq = num - PIC_IRQ_BASE;

	/* Check for spurious IRQ */
	if (15 == irq || 7 == irq) {
		uint16_t isr = PIC_get_isr();
		if (!(isr & (1<<irq))) {
			/* Spurious */
			if (15==irq) {
				/* EOI for cascade */
				PIC_eoi(2);
			}
			spurious++;
			return;
		}
	}

	inirq++;
	if (irq_table[irq]) {
		irq_table[irq](irq);
	}
	inirq--;

	PIC_eoi(irq);
}

#if 0
int irq_isblocked(int irq)
{
	uint16_t port;
	uint8_t value;

	if(irq < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq -= 8;
	}
	return inb(port) & (1 << irq);
}

void irq_start(int irq)
{
	PIC_set_mask(irq);
}

void irq_end(int irq)
{
	PIC_clear_mask(irq);
}
#endif
irq_func add_irq(int irq, irq_func handler)
{
	irq_func old = irq_table[irq];
	irq_table[irq] = handler;
	if (irq<8) {
		outb(PIC1_DATA, inb(PIC1_DATA) & ~(1<<irq));
	} else {
		outb(PIC1_DATA, inb(PIC1_DATA) & ~(1<<2));
		outb(PIC2_DATA, inb(PIC2_DATA) & ~(1<<(irq-8)));
	}
	return old;
}

typedef timerspec_t tickspec_t;
static void (*pit_expire)();
static tickspec_t ticks;
static int pit_lock[] = {0};

static void pit_set()
{
	if (ticks>65535) {
		outb(0x43, 0x30);
		outb(0x40, 0xff);
		outb(0x40, 0xff);
		ticks -= 65535;
	} else {
		outb(0x43, 0x30);
		outb(0x40, ticks & 0xff);
		outb(0x40, ticks >> 8);
		ticks = 0;
	}
}

static int pit_get()
{
	int count;

	outb(0x43, 0);
	count = inb(0x40);
	count += (inb(0x40)<<8);

	return count;
}

static void pit_timer_int(void * ignored)
{
	if (ticks) {
		pit_set();
	} else if (pit_expire) {
		pit_expire();
	}
}

interrupt_monitor_t * arch_timer_init()
{
	intr_add(0, pit_timer_int, 0);
	return interrupt_monitor_irq(0);
}

timerspec_t arch_timer_clear()
{
	const tickspec_t remaining = arch_timer_remaining();

	ticks = 0;
	pit_expire = 0;

	return remaining;
}

void arch_timer_set(void (*expire)(), timerspec_t usec)
{
	pit_expire = expire;
	ticks = PIT_HZ * usec / 1000000;

	pit_set();
}

timerspec_t arch_timer_remaining()
{
	const tickspec_t remaining = ticks + pit_get();

	return remaining * 1000000 / PIT_HZ;
}

void arch_idle()
{
	hlt();
}

uint32_t isa_inl(uint16_t port)
{
	return inl(port);
}

void isa_outl(uint16_t port, uint32_t data)
{
	outl(port, data);
}

uint16_t isa_inw(uint16_t port)
{
	return inw(port);
}

void isa_outw(uint16_t port, uint16_t data)
{
	outw(port, data);
}

uint8_t isa_inb(uint16_t port)
{
	return inb(port);
}

void isa_outb(uint16_t port, uint8_t data)
{
	outb(port, data);
}

void isa_init()
{
}
