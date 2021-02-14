#include "i386.h"

#if INTERFACE
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdnoreturn.h>

struct arch_trap_frame_t
{
	reg_t edi;
	reg_t esi;
	reg_t fp;
	reg_t esp_old;
	reg_t ebx;
	reg_t edx;
	reg_t ecx;
	reg_t eax;
	reg_t gs;
	reg_t fs;
	reg_t es;
	reg_t ds;
	reg_t errorcode;
	reg_t ebp;
	reg_t eip;
	reg_t cs;
	reg_t eflags;
	reg_t uesp;
	reg_t ss;
};

#define LITTLE_ENDIAN 1

#endif

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

extern void lcs();

static void lgdt(void * base, uint16_t size)
{
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) GDTR = { size, base };
 
    asm ( "lgdt %0" : : "m"(GDTR) );  // let the compiler choose an addressing mode
	lcs();
	asm volatile("movw %0, %%ds" : : "a"((short)0x10));
	asm volatile("movw %0, %%es" : : "a"((short)0x10));
	asm volatile("movw %0, %%fs" : : "a"((short)0x10));
	asm volatile("movw %0, %%gs" : : "a"((short)0x10));
	asm volatile("movw %0, %%ss" : : "a"((short)0x10));
}

static void ltr(uint16_t offset)
{
	asm volatile("ltr %0" : : "a"(offset));
}

void set_page_dir(page_t pgdir)
{
	asm volatile("movl %0, %%cr3" : : "a"(pgdir << ARCH_PAGE_SIZE_LOG2));
}

static int cli_level = 0;
void sti()
{
	if(!cli_level) {
		kernel_printk("cli_level is zero!\n");
		kernel_backtrace(logger_error);
	}
	if (0 == --cli_level) {
		asm volatile("sti");
	}
}

void cli()
{
	asm volatile("cli");
	cli_level++;
}

void hlt()
{	
	asm volatile("hlt");
}

void hang()
{
	while(1) {
		hlt();
	}
}

void reset()
{
	lidt(0,0);
	hlt();
}

void invlpg(const void* const m)
{
	/* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
	asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

static uint16_t idt[256][4];

#include "isr_labels_extern.h"
void * isr_labels[] = {
#include "isr_labels.h"
};

typedef void (*isr_t)(uint32_t i, arch_trap_frame_t * state);


void i386_set_idt( int i, void * p, uint16_t flags )
{
	uint32_t d = (uint32_t)p;
	idt[i][0] = d & 0xffff;
	idt[i][1] = 0x8;
	idt[i][2] = flags;
	idt[i][3] = d >> 16;
}

#define GDT_RW 0x00000200
#define GDT_EX 0x00000800
#define GDT_P  0x00008000
#define GDT_G  0x00800000
#define GDT_Sz 0x00400000
#define GDT_R3 0x00006000

static uint8_t gdt[][8] = {
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
};

static uint32_t tss[26] = { 0 };

static void encodeGdtEntry( uint8_t * entry, void * pbase, uint32_t size, uint8_t type )
{
	uint32_t base = (uint32_t)pbase;

	if (size>0xffff) {
		size >>= 12;
		entry[6] = 0xc0;
	} else {
		entry[6] = 0x40;
	}

	entry[0] = size & 0xff;
	entry[1] = (size>>8) & 0xff;
	entry[6] |= (size>>16) & 0xf;

	entry[2] = (base) & 0xff;
	entry[3] = (base >> 8) & 0xff;
	entry[4] = (base >> 16) & 0xff;
	entry[7] = (base >> 24) & 0xff;

	entry[5] = type;
}

static void i386_unhandled(uint32_t num, arch_trap_frame_t * state)
{
	kernel_panic("Unhandled exception: %d\n", num);
}

static void i386_de(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_db(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_nmi(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_bp(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_of(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_br(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_ud(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_nm(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_df(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_ts(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_np(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_ss(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_gp(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_pf(uint32_t num, arch_trap_frame_t * state)
{
	void * cr2;

	asm volatile("movl %%cr2, %0" : "=r"(cr2));
	vm_page_fault(cr2, state->errorcode & 0x2, state->errorcode & 0x4, state->errorcode & 0x1);
}

static void i386_mf(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_ac(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_mc(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_xm(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_ve(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

static void i386_sx(uint32_t num, arch_trap_frame_t * state)
{
	i386_unhandled(num, state);
}

#if INTERFACE
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#define ARCH_PAGE_ALIGN(p) ((void*)((uint32_t)(p) & (0xffffffff << ARCH_PAGE_SIZE_LOG2)))

struct arch_context_t {
	void * stack;
	jmp_buf state;
};

#endif

static thread_t initial;
static thread_t * current;

void arch_check_stack()
{
	char var;

	if ((&var - (char*)current->context.stack) < 128) {
		kernel_panic("Stack overflow");
	}
}

void arch_thread_mark(thread_t * thread)
{
	if (arch_get_thread() == thread) {
		setjmp(thread->context.state);
	}

	void ** esp = (void**)thread->context.state[1];
	void ** stacktop = (void**)((char*)thread->context.stack + ARCH_PAGE_SIZE);

	if (!arch_is_heap_pointer(esp)) {
		/* Bogus stack pointer - ignore */
		return;
	}

	/* Mark each potential address on the stack */
	slab_gc_mark_range(esp, stacktop);
	slab_gc_mark_block((void**)thread->context.state, sizeof(thread->context.state));
}

void arch_thread_finalize(thread_t * thread)
{
	page_heap_free(thread->context.stack);
}

static isr_t itable[256] = {
	i386_de, i386_db, i386_nmi, i386_bp,
	i386_of, i386_br, i386_ud, i386_nm,

	i386_df, 0, i386_ts, i386_np,
	i386_ss, i386_gp, i386_pf, 0,

	i386_mf, i386_ac, i386_mc, i386_xm,
	i386_ve, 0, 0, 0,

	0, 0, 0, 0,
	0, 0, i386_sx, 0,

	i386_irq, i386_irq, i386_irq, i386_irq,
	i386_irq, i386_irq, i386_irq, i386_irq,

	i386_irq, i386_irq, i386_irq, i386_irq,
	i386_irq, i386_irq, i386_irq, i386_irq,
	[0x80]=i386_syscall
};

#define PIC_IRQ_BASE    0x20
void i386_init()
{
	INIT_ONCE();

	int i;
	thread_t ** stackbase = ARCH_GET_VPAGE(&stackbase);

	encodeGdtEntry(gdt[0], 0, 0, 0);
	encodeGdtEntry(gdt[1], 0, 0xffffffff, 0x9a);
	encodeGdtEntry(gdt[2], 0, 0xffffffff, 0x92);
	encodeGdtEntry(gdt[3], 0, 0xffffffff, 0x9a | 0x60);
	encodeGdtEntry(gdt[4], 0, 0xffffffff, 0x92 | 0x60);
	encodeGdtEntry(gdt[5], tss, sizeof(tss), 0x89);
	tss[2] = 0x10;
	lgdt(gdt, sizeof(gdt));
	ltr(0x28);

	/* Default trap gates for all */
	for(i=0;i<sizeof(idt)/sizeof(idt[0]); i++) {
		i386_set_idt(i, isr_labels[i], 0x8f00);
	}

	/* int 0x80 - System call interface for user code */
	i386_set_idt(0x80, isr_labels[0x80], 0xef00);

	/* Configure interrupt gates for irqs */
	for(i=PIC_IRQ_BASE;i<PIC_IRQ_BASE+16; i++) {
		i386_set_idt(i, isr_labels[i], 0x8e00);
	}

	/* Configure the sensible interrupt table */

	lidt(idt,sizeof(idt));

	/* Craft the initial thread and stack */
	current = *stackbase = &initial;
	initial.context.stack = stackbase;
	initial.priority = THREAD_NORMAL;
	initial.state = THREAD_RUNNING;

	PIC_remap(PIC_IRQ_BASE, PIC_IRQ_BASE+8);
}

void arch_thread_init(thread_t * thread)
{
	INIT_ONCE();

	if (arch_thread_fork(thread)) {
		arch_thread_switch(thread);
	}
#if 0
	arch_get_thread()->as = tree_new(0, TREE_TREAP);
#endif
}

noreturn void arch_panic(const char * fmt, va_list ap)
{
	cli();
	stream_vprintf(console_stream(), fmt, ap);
	while(1) {
		hlt();
	}
}

static void unhandled_isr(uint32_t num, arch_trap_frame_t * state)
{
	kernel_printk("UNHANDLED ISR %d\n", num);
}

void i386_isr(uint32_t num, arch_trap_frame_t * state)
{
	isr_t isr = itable[num] ? itable[num] : unhandled_isr;

	isr(num, state);

	/* Do any post-return processing */
	intr_beforereturn();

	/* For return to usermode, do any processing such as delivering signals */
	if (state->cs == 0x1b) {
		intr_beforereturntouser();
	}
}

thread_t * arch_get_thread()
{
#if 0
	thread_t ** stackbase = ARCH_GET_VPAGE(&stackbase);

	return *stackbase;
#endif
	return current;
}

int arch_thread_fork(thread_t * dest)
{
	assert(!cli_level);
	/* Allocate the stack */
	thread_t * source = arch_get_thread();
	/* Top level copy */
	memcpy(dest, source, sizeof(*dest));

	/* Stacks */
	uint32_t * dpage = (uint32_t*)ARCH_GET_VPAGE(dest->context.stack = page_heap_alloc());
	uint32_t * spage = (uint32_t*)ARCH_GET_VPAGE(source->context.stack);

	/* Set pointer to thread */
	dpage[0] = (uintptr_t)dest;

	/* Copy the source thread stack */
	for(int i=1; i<ARCH_PAGE_SIZE/sizeof(*dpage); i++) {
		if (ARCH_PTRI_BASE(spage[i]) == ARCH_PTRI_BASE(spage)) {
			/* Adjust pointer */
			dpage[i] = (uintptr_t)dpage | ARCH_PTRI_OFFSET(spage[i]);
		} else {
			dpage[i] = spage[i];
		}
	}

	if (setjmp(dest->context.state)) {
		/* Child thread */
		return 0;
	}

	/* Adjust destination context */
	for(int i=0; i<sizeof(dest->context.state)/sizeof(dest->context.state[0]); i++) {
		if (ARCH_PTRI_BASE(dest->context.state[i]) == ARCH_PTRI_BASE(spage)) {
			dest->context.state[i] = (uintptr_t)dpage | ARCH_PTRI_OFFSET(dest->context.state[i]);
		}
	}

	/* Adjust TLS */
	for(int i=0; i<sizeof(dest->tls)/sizeof(dest->tls[0]); i++) {
		if (ARCH_PTRI_BASE(dest->tls[i]) == ARCH_PTRI_BASE(spage)) {
			dest->tls[i] = (void*)((uintptr_t)dpage | ARCH_PTRI_OFFSET(dest->tls[i]));
		}
	}

	return 1;
}

void arch_thread_switch(thread_t * thread)
{
	thread_t * old = arch_get_thread();

	if (old == thread) {
		thread->state = THREAD_RUNNING;
		return;
	}

	if (old->state == THREAD_RUNNING) {
		old->state = THREAD_RUNNABLE;
	}

	if (old->process != thread->process) {
		if (thread->process) {
			vmap_set_asid(thread->process->as);
		} else {
			vmap_set_asid(0);
		}
	}

	if (0 == setjmp(old->context.state)) {
		if (thread->state == THREAD_RUNNABLE) {
			thread->state = THREAD_RUNNING;
		}
		tss[1] = (uint32_t)thread->context.stack + ARCH_PAGE_SIZE;
		current = thread;
		longjmp(thread->context.state, 1);
	}
}

static int arch_is_text(void * p)
{
	char * cp = p;
	extern char code_start[];
	extern char code_end[];

	return cp >= code_start && cp < code_end;
}

void ** arch_thread_backtrace(void ** backtrace, int levels)
{
	if (0 == backtrace) {
		backtrace = tmalloc(sizeof(*backtrace)*levels+1);
	}
	thread_t * thread = arch_get_thread();
	setjmp(thread->context.state);
	void * stacktop = (void**)((char*)thread->context.stack + ARCH_PAGE_SIZE);
	void ** bp = (void**)thread->context.state[2];
	int i;

	for(i=0; i<levels && bp > (void**)thread->context.state[1] && (void*)bp<stacktop; ) {
		void * ret = bp[1];
		if(arch_is_text(ret)) {
			backtrace[i++] = ret;
		}
		bp = bp[0];
	}
	if (i<levels-1) {
		backtrace[i] = 0;
	}
	
	return backtrace;
}

int arch_atomic_postinc(int * p)
{
	int i;
	cli();
	i = *p;
	*p = i+1;
	sti();

	return i;
}

int arch_spin_trylock(spin_t * p)
{
	cli();
	if (atomic_flag_test_and_set(p)) {
		sti();
		return 0;
	}
	barrier();
	return 1;
}

void arch_spin_lock(spin_t * p)
{
	while(1) {
		if (arch_spin_trylock(p)) {
			return;
		}
	}
}

void arch_spin_unlock(spin_t * p)
{
	atomic_flag_clear(p);
	barrier();
	sti();
}

#if 0
void * arch_user_stack()
{
}
#endif

char * arch_user_stack_pushstr(char * oldsp, char * str)
{
	int len = strlen(str);
#if 0
	char * newsp = (char*)PTR_ALIGN(oldsp-len, sizeof(int32_t));

	strncpy(newsp, str, len);

	return newsp;
#endif
	return arch_user_stack_mempcy(oldsp, str, len+1);
}

char * arch_user_stack_mempcy(char * oldsp, void * src, size_t len)
{
	char * newsp = (char*)PTR_ALIGN(oldsp-len, sizeof(int32_t));

	return memcpy(newsp, src, len);
}

#if INTERFACE

void arch_startuser(void * start, void * tos);
void arch_altstack(void (*f)(void), void * s);

/*
 * Sizes
 */
#define ARCH_PAGE_SIZE_LOG2 12
#define ARCH_PAGE_SIZE (1<<ARCH_PAGE_SIZE_LOG2)
#define ARCH_PAGE_TABLE_SIZE_LOG2 20
#define ARCH_PAGE_TABLE_SIZE (1<<ARCH_PAGE_TABLE_SIZE_LOG2)

/*
 *
 */
#define ARCH_PTRI_OFFSET_MASK (ARCH_PAGE_SIZE-1)
#define ARCH_PTRI_OFFSET(p) ((uintptr_t)(p) & (ARCH_PTRI_OFFSET_MASK))
#define ARCH_PTRI_BASE(p) ((uintptr_t)(p) & ~(ARCH_PTRI_OFFSET_MASK))
#define ARCH_GET_VPAGE(p) ((void*)ARCH_PTRI_BASE(p))

#endif
