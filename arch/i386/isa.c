#include <stdint.h>
#include <stdarg.h>

#include "isa.h"

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

static volatile uint32_t irq_flag = 0;
static void isa_irq(uint32_t num, uint32_t * state)
{
	int irq = num - PIC_IRQ_BASE;

	irq_flag |= (1 << irq);

	if (irq_table[irq]) {
		irq_table[irq]();
	}

	PIC_eoi(irq);
}

void i386_irq(uint32_t num, uint32_t * state)
{
	int irq = num - PIC_IRQ_BASE;

	irq_flag |= (1 << irq);

	if (irq_table[irq]) {
		irq_table[irq]();
	}

	PIC_eoi(irq);
}

irq_func add_irq(int irq, irq_func handler)
{
	irq_func old = irq_table[irq];
	irq_table[irq] = handler;
	return old;
}

static int wait_irq()
{
	int irq = 0;

	while(0 == irq_flag) {
		hlt();
	}

	for(; irq<16; irq++) {
		int mask = 1<<irq;
		if (irq_flag & mask) {
			cli();
			irq_flag &= ~mask;
			sti();
			return irq;
		}
	}

	return 0;
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
	} else {
		outb(0x43, 0x30);
		outb(0x40, ticks & 0xff);
		outb(0x40, ticks >> 8);
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

static void pit_timer_int()
{
	SPIN_AUTOLOCK(pit_lock) {
		if (ticks>65535) {
			ticks -= 65535;
			pit_set();
		} else {
			ticks = 0;
			if (pit_expire) {
				spin_unlock(pit_lock);
				pit_expire();
				spin_lock(pit_lock);
			}
		}
	}
}

static void pit_timer_set(void (*expire)(), timerspec_t usec)
{
	SPIN_AUTOLOCK(pit_lock) {
		pit_expire = expire;
		ticks = 1193180 * usec / 1000000;

		pit_set();
	}
}

static timerspec_t pit_timer_clear()
{
	tickspec_t remaining;

	SPIN_AUTOLOCK(pit_lock) {
		if (ticks>65535) {
			remaining = ticks - 65535 + pit_get();
		} else {
			remaining = pit_get();
		}

		ticks = 0;
		pit_expire = 0;
	}

	return remaining * 1000000 / 1193180;
}

timer_ops_t * arch_timer_ops()
{
	static timer_ops_t ops = {
		timer_set: pit_timer_set,
		timer_clear: pit_timer_clear
	};

	add_irq(0, pit_timer_int);

	return &ops;
}

void arch_idle()
{
	int i = 0;
	static char wheel[] = {'|', '/', '-', '\\' };
	while(1) {
		int irq = wait_irq();

		switch(irq) {
		case 0:
			kernel_printk("%c\r", wheel[i]);
			i=(i+1)&3;
			break;
		default:
			kernel_printk("%d\n", irq);
			break;
		}
		for(uint8_t scancode = keyq_get(); scancode; scancode = keyq_get()) {
			kernel_printk("%x\n", scancode);
			if (0x13 == scancode) {
				reset();
			}
		}
		thread_lock(arch_idle);
		thread_gc();
		thread_unlock(arch_idle);
		thread_yield();
	}
	kernel_panic("idle finished");
}

void isa_init()
{
}
