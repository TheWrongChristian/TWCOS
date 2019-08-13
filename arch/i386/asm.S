// Declare constants used for creating a multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

// Declare a header as in the Multiboot Standard. We put this into a special
// section so we can force the header to be in the start of the final program.
// You don't need to understand all these details as it is just magic values that
// is documented in the multiboot standard. The bootloader will search for this
// magic sequence and recognize us as a multiboot kernel.
.section .bootstrap_data
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .data
.global idt
idt:
	.rept 256
	.word 0
	.word 0
	.word 0
	.word 0
	.endr

// Currently the stack pointer register (esp) points at anything and using it may
// cause massive harm. Instead, we'll provide our own stack. We will allocate
// room for a small temporary stack by creating a symbol at the bottom of it,
// then allocating 16384 bytes for it, and finally creating a symbol at the top.
.section .bootstrap_stack, "aw", @nobits
bootstrap_stack_bottom:
.skip 1024 # 4 KiB
bootstrap_stack_top:

.section .data
stack_bottom:
.skip 1024 # 4 KiB
stack_top:


// Bootstrap page table
.section .bootstrap_pgtbl, "aw", @nobits
.align 4096
.global pg_dir
pg_dir:
	.rept 1024
	.word 0
	.endr

.align 4096
.global pt_00000000
pt_00000000:
	.rept 1024
	.word 0
	.endr

// The linker script specifies _start as the entry point to the kernel and the
// bootloader will jump to this position once the kernel has been loaded. It
// doesn't make sense to return from this function as the bootloader is gone.
.section .bootstrap_code
.global _start
.type _start, @function
_start:
	# Welcome to kernel mode! We now have sufficient code for the bootloader to
	# load and run our operating system. It doesn't do anything interesting yet.
	# Perhaps we would like to call printf("Hello, World\n"). You should now
	# realize one of the profound truths about kernel mode: There is nothing
	# there unless you provide it yourself. There is no printf function. There
	# is no <stdio.h> header. If you want a function, you will have to code it
	# yourself. And that is one of the best things about kernel development:
	# you get to make the entire system yourself. You have absolute and complete
	# power over the machine, there are no security restrictions, no safe
	# guards, no debugging mechanisms, there is nothing but what you build.

	# By now, you are perhaps tired of assembly language. You realize some
	# things simply cannot be done in C, such as making the multiboot header in
	# the right section and setting up the stack. However, you would like to
	# write the operating system in a higher level language, such as C or C++.
	# To that end, the next task is preparing the processor for execution of
	# such code. C doesn't expect much at this point and we only need to set up
	# a stack. Note that the processor is not fully initialized yet and stuff
	# such as floating point instructions are not available yet.

	# To set up a stack, we simply set the esp register to point to the top of
	# our stack (as it grows downwards).
	movl $bootstrap_stack_top, %esp

	# We are now ready to actually execute C code.
	# Save multiboot info for later use
	pushl %ebx
	pushl %eax
	call multiboot_init
	addl $8, %esp

	# Initialize the page table and page directory
	call bootstrap_paging_init

	# Turn on paging
	movl $pg_dir, %eax
	movl %eax, %cr3
	movl %cr4, %eax
	orl $0x00000008, %eax
	movl %eax, %cr4
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0
	jmp .highmem

.global user_start
.type user_start, @function
user_start:
	movl $2, %eax
	int $128
	test %eax,%eax
	je 1f

	# parent
	movl $1, %eax
	movl $25, %ebx
	int $128
	# child
1:
	movl $1, %eax
	movl $25, %ebx
	int $128

.section .text
.highmem:
	movl $stack_top, %esp

	# We'll create a C entry point called kernel_main and call it here.
	call kernel_main

	# In case the function returns, we'll want to put the computer into an
	# infinite loop. To do that, we use the clear interrupt ('cli') instruction
	# to disable interrupts, the halt instruction ('hlt') to stop the CPU until
	# the next interrupt arrives, and jumping to the halt instruction if it ever
	# continues execution, just to be safe. We will create a local label rather
	# than real symbol and jump to there endlessly.
	cli
0:
	hlt
	jmp 0b

.global switch
switch:
	movl %esp, %eax
	ret

.global isr
.type isr, @function
isr:
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %ds
	# We have an error code - pop it
	addl $4, %esp
	iret

.global lcs
.type isr, @function
lcs:
	ljmp $0x8, $1f
1:
	ret

.global arch_startuser
.type arch_startuser, @function
arch_startuser:
	push %ebp
	mov %esp,%ebp

	# data segment
	mov $0x23,%ecx
	mov %ecx, %ds
	mov %ecx, %es
	mov %ecx, %fs
	mov %ecx, %gs
	# stack segment
	push %ecx

	# stack pointer
	mov 12(%ebp),%ecx
	push %ecx

	# eflags
	pushl $0

	# user cs
	pushl $0x1b

	# user pc
	mov 8(%ebp), %ecx
	push %ecx

	xor %eax, %eax
	mov %eax, %ebx
	mov %eax, %ecx
	mov %eax, %edx
	mov %eax, %ebp
	mov %eax, %esi
	mov %eax, %edi

	# Jump to user mode
	iret

.global arch_altstack
.type arch_altstack, @function
arch_altstack:
	push %ebp
	mov %esp,%ebp

	# called pc
	mov 8(%ebp), %edx

	# stack pointer
	mov 12(%ebp),%esp
	push %ebp
	call %edx
	pop %esp
	pop %ebp
	ret

#include "isr.inc"
