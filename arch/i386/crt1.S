	.section .text
	.global _start
_start:
	call cppstart
	movl %eax, %ebx
	movl $1, %eax
	int $0x80
