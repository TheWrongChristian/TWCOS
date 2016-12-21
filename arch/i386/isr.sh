#!/bin/sh

errorcode_isr ()
{
	cat <<EOF
.global isr_$1
isr_$1:
	pushl %ds
	pushal
	pushl %esp
	pushl \$$1
	jmp isr

EOF
}

isr () 
{
	cat <<EOF
.global isr_$1
isr_$1:
	push \$0
	pushl %ds
	pushal
	pushl %esp
	pushl \$$1
	jmp isr

EOF
}

for i in $(seq 0 255)
do
	if [ $i -eq 8 -o $i -gt 9 -a $i -lt 15 ]
	then
		errorcode_isr $i
	else
		isr $i
	fi
done
