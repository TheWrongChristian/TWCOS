#!/bin/sh

isr_body()
{
	cat <<EOF
	pushl %ds
	pushal
	pushl %esp
	pushl \$$1
	cld
	movw \$0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl \$8, %esp
	popal
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

EOF
}

errorcode_isr ()
{
	cat <<EOF
.global isr_$1
.type isr_$1, @function
isr_$1:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
EOF
	isr_body $1
}

noerrorcode_isr () 
{
	cat <<EOF
.global isr_$1
.type isr_$1, @function
isr_$1:
	pushl %ebp
	movl %esp, %ebp
EOF
	isr_body $1
}

for i in $(seq 0 255)
do
	if [ $i -eq 8 -o $i -gt 9 -a $i -lt 15 ]
	then
		errorcode_isr $i
	else
		noerrorcode_isr $i
	fi
done
