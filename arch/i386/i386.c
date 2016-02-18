#include <stdint.h>

#include "i386.h"

/* Basic port I/O */
void outb(uint16_t port, uint8_t v)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}
uint8_t inb(uint16_t port)
{
	uint8_t v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

void outw(uint16_t port, uint16_t v)
{
	asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}
uint16_t inw(uint16_t port)
{
	uint16_t v;
	asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

void outl(uint16_t port, uint32_t v)
{
	asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}
uint32_t inl(uint16_t port)
{
	uint32_t v;
	asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

void cpuid(int code, uint32_t* a, uint32_t* d)
{
	asm volatile ( "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx" );
}

static void lidt(void* base, uint16_t size)
{   // This function works in 32 and 64bit mode
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };
 
    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void sti()
{
	asm volatile("sti");
}

void cli()
{
	asm volatile("cli");
}

void hlt()
{
	asm volatile("hlt");
}

void invlpg(void* m)
{
	/* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
	asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

extern uint16_t idt[256][4];

#include "isr_labels_extern.h"
void * isr_labels[] = {
#include "isr_labels.h"
};

void i386_set_idt( int i, void * p, uint16_t flags )
{
	uint32_t d = (uint32_t)p;
	idt[i][0] = d & 0xffff;
	idt[i][1] = 0x8;
	idt[i][2] = flags;
	idt[i][3] = d >> 16;
}

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define PIC1		0x20
#define PIC2		0xA0   
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)
#define PIC_EOI		0x20

 
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

static void PIC_eoi(int irq)
{
	if(irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);

	outb(PIC1_COMMAND,PIC_EOI);
}

static void PIC_remap(int offset1, int offset2)
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

void i386_init()
{
	int i;

	/* Default trap gates for all */
	for(i=0;i<sizeof(idt)/sizeof(idt[0]); i++) {
		i386_set_idt(i, isr_labels[i], 0x8f00);
	}

	/* Configure interrupt gates for irqs */
	for(i=32;i<48; i++) {
		i386_set_idt(i, isr_labels[i], 0x8e00);
	}

	lidt(idt,sizeof(idt));

	PIC_remap(32, 40);

	sti();
}

void arch_idle()
{
	while(1) {
		hlt();
	}
}

int i386_isr(uint32_t num, uint32_t * state)
{
	int errorcode = 0;

	if (num<32) {
		kernel_printk("ISR %d\n", num);
	} else if (num<48) {
		int irq = num - 32;
		kernel_printk("IRQ %d\n", irq);
		PIC_eoi(irq);
	} else {
		kernel_printk("ISR %d\n", num);
	}

	return errorcode;
}
