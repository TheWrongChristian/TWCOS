# Code for setjmp/longjmp
.section .text
.global setjmp
.type setjmp, @function
setjmp:
	# jmp_buf ptr
	movl 4(%esp), %eax
	movl (%esp), %ecx	# return address - eip
	movl %ecx, (%eax)	# return address - eip
	movl %esp, 4(%eax)
	movl %ebp, 8(%eax)
	movl %ebx, 12(%eax)
	movl %esi, 16(%eax)
	movl %edi, 20(%eax)
	xor %eax, %eax
	ret

.global longjmp
.type longjmp, @function
longjmp:
	# jmp_buf ptr
	movl 4(%esp), %ecx
	# Return value
	movl 8(%esp), %eax

	movl 4(%ecx), %esp
	movl 8(%ecx), %ebp
	movl 12(%ecx), %ebx
	movl 16(%ecx), %esi
	movl 20(%ecx), %edi
	movl (%ecx), %ecx

	# "return" to original address
	jmp *%ecx
