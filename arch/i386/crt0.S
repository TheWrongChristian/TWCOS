	.section .text
	.global _start
_start:
	call cstart
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
