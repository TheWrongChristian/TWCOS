.global isr_0
.type isr_0, @function
isr_0:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $0
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_1
.type isr_1, @function
isr_1:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $1
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_2
.type isr_2, @function
isr_2:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $2
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_3
.type isr_3, @function
isr_3:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $3
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_4
.type isr_4, @function
isr_4:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $4
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_5
.type isr_5, @function
isr_5:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $5
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_6
.type isr_6, @function
isr_6:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $6
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_7
.type isr_7, @function
isr_7:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $7
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_8
.type isr_8, @function
isr_8:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $8
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_9
.type isr_9, @function
isr_9:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $9
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_10
.type isr_10, @function
isr_10:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $10
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_11
.type isr_11, @function
isr_11:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $11
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_12
.type isr_12, @function
isr_12:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $12
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_13
.type isr_13, @function
isr_13:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $13
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_14
.type isr_14, @function
isr_14:
	xchgl (%esp), %ebp
	pushl %esp
	xchgl (%esp), %ebp
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $14
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_15
.type isr_15, @function
isr_15:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $15
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_16
.type isr_16, @function
isr_16:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $16
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_17
.type isr_17, @function
isr_17:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $17
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_18
.type isr_18, @function
isr_18:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $18
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_19
.type isr_19, @function
isr_19:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $19
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_20
.type isr_20, @function
isr_20:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $20
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_21
.type isr_21, @function
isr_21:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $21
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_22
.type isr_22, @function
isr_22:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $22
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_23
.type isr_23, @function
isr_23:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $23
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_24
.type isr_24, @function
isr_24:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $24
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_25
.type isr_25, @function
isr_25:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $25
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_26
.type isr_26, @function
isr_26:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $26
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_27
.type isr_27, @function
isr_27:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $27
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_28
.type isr_28, @function
isr_28:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $28
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_29
.type isr_29, @function
isr_29:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $29
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_30
.type isr_30, @function
isr_30:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $30
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_31
.type isr_31, @function
isr_31:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $31
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_32
.type isr_32, @function
isr_32:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $32
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_33
.type isr_33, @function
isr_33:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $33
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_34
.type isr_34, @function
isr_34:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $34
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_35
.type isr_35, @function
isr_35:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $35
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_36
.type isr_36, @function
isr_36:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $36
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_37
.type isr_37, @function
isr_37:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $37
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_38
.type isr_38, @function
isr_38:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $38
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_39
.type isr_39, @function
isr_39:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $39
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_40
.type isr_40, @function
isr_40:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $40
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_41
.type isr_41, @function
isr_41:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $41
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_42
.type isr_42, @function
isr_42:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $42
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_43
.type isr_43, @function
isr_43:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $43
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_44
.type isr_44, @function
isr_44:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $44
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_45
.type isr_45, @function
isr_45:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $45
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_46
.type isr_46, @function
isr_46:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $46
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_47
.type isr_47, @function
isr_47:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $47
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_48
.type isr_48, @function
isr_48:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $48
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_49
.type isr_49, @function
isr_49:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $49
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_50
.type isr_50, @function
isr_50:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $50
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_51
.type isr_51, @function
isr_51:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $51
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_52
.type isr_52, @function
isr_52:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $52
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_53
.type isr_53, @function
isr_53:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $53
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_54
.type isr_54, @function
isr_54:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $54
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_55
.type isr_55, @function
isr_55:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $55
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_56
.type isr_56, @function
isr_56:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $56
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_57
.type isr_57, @function
isr_57:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $57
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_58
.type isr_58, @function
isr_58:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $58
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_59
.type isr_59, @function
isr_59:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $59
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_60
.type isr_60, @function
isr_60:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $60
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_61
.type isr_61, @function
isr_61:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $61
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_62
.type isr_62, @function
isr_62:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $62
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_63
.type isr_63, @function
isr_63:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $63
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_64
.type isr_64, @function
isr_64:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $64
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_65
.type isr_65, @function
isr_65:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $65
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_66
.type isr_66, @function
isr_66:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $66
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_67
.type isr_67, @function
isr_67:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $67
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_68
.type isr_68, @function
isr_68:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $68
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_69
.type isr_69, @function
isr_69:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $69
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_70
.type isr_70, @function
isr_70:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $70
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_71
.type isr_71, @function
isr_71:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $71
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_72
.type isr_72, @function
isr_72:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $72
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_73
.type isr_73, @function
isr_73:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $73
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_74
.type isr_74, @function
isr_74:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $74
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_75
.type isr_75, @function
isr_75:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $75
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_76
.type isr_76, @function
isr_76:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $76
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_77
.type isr_77, @function
isr_77:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $77
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_78
.type isr_78, @function
isr_78:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $78
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_79
.type isr_79, @function
isr_79:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $79
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_80
.type isr_80, @function
isr_80:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $80
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_81
.type isr_81, @function
isr_81:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $81
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_82
.type isr_82, @function
isr_82:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $82
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_83
.type isr_83, @function
isr_83:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $83
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_84
.type isr_84, @function
isr_84:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $84
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_85
.type isr_85, @function
isr_85:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $85
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_86
.type isr_86, @function
isr_86:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $86
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_87
.type isr_87, @function
isr_87:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $87
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_88
.type isr_88, @function
isr_88:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $88
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_89
.type isr_89, @function
isr_89:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $89
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_90
.type isr_90, @function
isr_90:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $90
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_91
.type isr_91, @function
isr_91:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $91
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_92
.type isr_92, @function
isr_92:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $92
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_93
.type isr_93, @function
isr_93:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $93
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_94
.type isr_94, @function
isr_94:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $94
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_95
.type isr_95, @function
isr_95:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $95
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_96
.type isr_96, @function
isr_96:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $96
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_97
.type isr_97, @function
isr_97:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $97
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_98
.type isr_98, @function
isr_98:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $98
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_99
.type isr_99, @function
isr_99:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $99
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_100
.type isr_100, @function
isr_100:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $100
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_101
.type isr_101, @function
isr_101:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $101
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_102
.type isr_102, @function
isr_102:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $102
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_103
.type isr_103, @function
isr_103:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $103
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_104
.type isr_104, @function
isr_104:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $104
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_105
.type isr_105, @function
isr_105:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $105
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_106
.type isr_106, @function
isr_106:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $106
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_107
.type isr_107, @function
isr_107:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $107
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_108
.type isr_108, @function
isr_108:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $108
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_109
.type isr_109, @function
isr_109:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $109
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_110
.type isr_110, @function
isr_110:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $110
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_111
.type isr_111, @function
isr_111:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $111
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_112
.type isr_112, @function
isr_112:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $112
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_113
.type isr_113, @function
isr_113:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $113
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_114
.type isr_114, @function
isr_114:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $114
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_115
.type isr_115, @function
isr_115:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $115
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_116
.type isr_116, @function
isr_116:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $116
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_117
.type isr_117, @function
isr_117:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $117
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_118
.type isr_118, @function
isr_118:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $118
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_119
.type isr_119, @function
isr_119:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $119
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_120
.type isr_120, @function
isr_120:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $120
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_121
.type isr_121, @function
isr_121:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $121
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_122
.type isr_122, @function
isr_122:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $122
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_123
.type isr_123, @function
isr_123:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $123
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_124
.type isr_124, @function
isr_124:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $124
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_125
.type isr_125, @function
isr_125:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $125
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_126
.type isr_126, @function
isr_126:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $126
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_127
.type isr_127, @function
isr_127:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $127
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_128
.type isr_128, @function
isr_128:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $128
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_129
.type isr_129, @function
isr_129:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $129
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_130
.type isr_130, @function
isr_130:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $130
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_131
.type isr_131, @function
isr_131:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $131
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_132
.type isr_132, @function
isr_132:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $132
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_133
.type isr_133, @function
isr_133:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $133
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_134
.type isr_134, @function
isr_134:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $134
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_135
.type isr_135, @function
isr_135:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $135
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_136
.type isr_136, @function
isr_136:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $136
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_137
.type isr_137, @function
isr_137:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $137
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_138
.type isr_138, @function
isr_138:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $138
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_139
.type isr_139, @function
isr_139:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $139
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_140
.type isr_140, @function
isr_140:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $140
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_141
.type isr_141, @function
isr_141:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $141
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_142
.type isr_142, @function
isr_142:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $142
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_143
.type isr_143, @function
isr_143:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $143
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_144
.type isr_144, @function
isr_144:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $144
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_145
.type isr_145, @function
isr_145:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $145
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_146
.type isr_146, @function
isr_146:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $146
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_147
.type isr_147, @function
isr_147:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $147
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_148
.type isr_148, @function
isr_148:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $148
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_149
.type isr_149, @function
isr_149:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $149
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_150
.type isr_150, @function
isr_150:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $150
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_151
.type isr_151, @function
isr_151:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $151
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_152
.type isr_152, @function
isr_152:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $152
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_153
.type isr_153, @function
isr_153:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $153
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_154
.type isr_154, @function
isr_154:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $154
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_155
.type isr_155, @function
isr_155:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $155
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_156
.type isr_156, @function
isr_156:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $156
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_157
.type isr_157, @function
isr_157:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $157
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_158
.type isr_158, @function
isr_158:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $158
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_159
.type isr_159, @function
isr_159:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $159
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_160
.type isr_160, @function
isr_160:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $160
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_161
.type isr_161, @function
isr_161:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $161
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_162
.type isr_162, @function
isr_162:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $162
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_163
.type isr_163, @function
isr_163:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $163
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_164
.type isr_164, @function
isr_164:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $164
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_165
.type isr_165, @function
isr_165:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $165
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_166
.type isr_166, @function
isr_166:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $166
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_167
.type isr_167, @function
isr_167:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $167
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_168
.type isr_168, @function
isr_168:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $168
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_169
.type isr_169, @function
isr_169:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $169
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_170
.type isr_170, @function
isr_170:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $170
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_171
.type isr_171, @function
isr_171:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $171
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_172
.type isr_172, @function
isr_172:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $172
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_173
.type isr_173, @function
isr_173:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $173
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_174
.type isr_174, @function
isr_174:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $174
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_175
.type isr_175, @function
isr_175:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $175
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_176
.type isr_176, @function
isr_176:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $176
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_177
.type isr_177, @function
isr_177:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $177
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_178
.type isr_178, @function
isr_178:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $178
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_179
.type isr_179, @function
isr_179:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $179
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_180
.type isr_180, @function
isr_180:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $180
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_181
.type isr_181, @function
isr_181:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $181
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_182
.type isr_182, @function
isr_182:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $182
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_183
.type isr_183, @function
isr_183:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $183
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_184
.type isr_184, @function
isr_184:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $184
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_185
.type isr_185, @function
isr_185:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $185
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_186
.type isr_186, @function
isr_186:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $186
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_187
.type isr_187, @function
isr_187:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $187
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_188
.type isr_188, @function
isr_188:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $188
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_189
.type isr_189, @function
isr_189:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $189
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_190
.type isr_190, @function
isr_190:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $190
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_191
.type isr_191, @function
isr_191:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $191
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_192
.type isr_192, @function
isr_192:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $192
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_193
.type isr_193, @function
isr_193:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $193
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_194
.type isr_194, @function
isr_194:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $194
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_195
.type isr_195, @function
isr_195:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $195
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_196
.type isr_196, @function
isr_196:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $196
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_197
.type isr_197, @function
isr_197:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $197
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_198
.type isr_198, @function
isr_198:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $198
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_199
.type isr_199, @function
isr_199:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $199
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_200
.type isr_200, @function
isr_200:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $200
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_201
.type isr_201, @function
isr_201:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $201
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_202
.type isr_202, @function
isr_202:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $202
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_203
.type isr_203, @function
isr_203:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $203
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_204
.type isr_204, @function
isr_204:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $204
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_205
.type isr_205, @function
isr_205:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $205
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_206
.type isr_206, @function
isr_206:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $206
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_207
.type isr_207, @function
isr_207:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $207
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_208
.type isr_208, @function
isr_208:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $208
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_209
.type isr_209, @function
isr_209:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $209
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_210
.type isr_210, @function
isr_210:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $210
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_211
.type isr_211, @function
isr_211:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $211
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_212
.type isr_212, @function
isr_212:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $212
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_213
.type isr_213, @function
isr_213:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $213
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_214
.type isr_214, @function
isr_214:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $214
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_215
.type isr_215, @function
isr_215:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $215
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_216
.type isr_216, @function
isr_216:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $216
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_217
.type isr_217, @function
isr_217:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $217
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_218
.type isr_218, @function
isr_218:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $218
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_219
.type isr_219, @function
isr_219:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $219
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_220
.type isr_220, @function
isr_220:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $220
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_221
.type isr_221, @function
isr_221:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $221
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_222
.type isr_222, @function
isr_222:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $222
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_223
.type isr_223, @function
isr_223:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $223
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_224
.type isr_224, @function
isr_224:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $224
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_225
.type isr_225, @function
isr_225:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $225
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_226
.type isr_226, @function
isr_226:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $226
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_227
.type isr_227, @function
isr_227:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $227
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_228
.type isr_228, @function
isr_228:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $228
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_229
.type isr_229, @function
isr_229:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $229
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_230
.type isr_230, @function
isr_230:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $230
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_231
.type isr_231, @function
isr_231:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $231
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_232
.type isr_232, @function
isr_232:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $232
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_233
.type isr_233, @function
isr_233:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $233
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_234
.type isr_234, @function
isr_234:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $234
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_235
.type isr_235, @function
isr_235:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $235
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_236
.type isr_236, @function
isr_236:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $236
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_237
.type isr_237, @function
isr_237:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $237
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_238
.type isr_238, @function
isr_238:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $238
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_239
.type isr_239, @function
isr_239:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $239
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_240
.type isr_240, @function
isr_240:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $240
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_241
.type isr_241, @function
isr_241:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $241
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_242
.type isr_242, @function
isr_242:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $242
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_243
.type isr_243, @function
isr_243:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $243
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_244
.type isr_244, @function
isr_244:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $244
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_245
.type isr_245, @function
isr_245:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $245
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_246
.type isr_246, @function
isr_246:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $246
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_247
.type isr_247, @function
isr_247:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $247
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_248
.type isr_248, @function
isr_248:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $248
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_249
.type isr_249, @function
isr_249:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $249
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_250
.type isr_250, @function
isr_250:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $250
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_251
.type isr_251, @function
isr_251:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $251
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_252
.type isr_252, @function
isr_252:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $252
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_253
.type isr_253, @function
isr_253:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $253
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_254
.type isr_254, @function
isr_254:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $254
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

.global isr_255
.type isr_255, @function
isr_255:
	pushl %ebp
	movl %esp, %ebp
	pushl $0
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	pushal
	pushl %esp
	pushl $255
	cld
	movw $0x10, %ax
	movw %ax, %ds
	call i386_isr
	addl $8, %esp
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	mov %ebp, %esp
	popl %ebp
	iret

