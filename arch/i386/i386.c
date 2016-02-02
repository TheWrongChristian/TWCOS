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

void invlpg(void* m)
{
	/* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
	asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

extern uint16_t idt[4][256];

static void i386_init()
{
	
}
