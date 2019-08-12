ARCH_SRCS_S := $(subdir)/asm.S $(subdir)/setjmp.S
ARCH_SRCS_C := $(subdir)/console.c $(subdir)/multiboot.c $(subdir)/init.c $(subdir)/vmap.c $(subdir)/i386.c $(subdir)/pci.c $(subdir)/isa.c $(subdir)/syscall.c # $(subdir)/usyscall.c
ARCH_USYSCALL_C := $(subdir)/usyscall.c
ARCH_CRT0_S := $(subdir)/crt0.S
ARCH_CRT0_O := $(ARCH_CRT0_S:.S=.o)
SRCS_S += $(ARCH_SRCS_S)
SRCS_C += $(ARCH_SRCS_C)

$(subdir)/asm.S: $(subdir)/isr.inc
$(subdir)/isr.inc: $(subdir)/isr.sh
	sh $< > $@

LD_SCRIPT := $(subdir)/linker.ld

$(subdir)/kernel: $(OBJS) $(LD_SCRIPT)
	$(CC) -T $(LD_SCRIPT) -o $@ -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

# all:: $(TOP)/lib/libsyscall.a $(TOP)/lib/crt0.o
all:: $(TOP)/lib/crt0.o

USERLIBS += $(TOP)/lib/crt0.o

# $(TOP)/lib/libsyscall.a: $(USYSCALL_SRCS_O)
# 	$(AR) rcs $@ $(USYSCALL_SRCS_O)

$(TOP)/lib/crt0.o: $(ARCH_CRT0_O)
	$(CP) $< $@

clean::
	$(RM) $(TOP)/lib/libsyscall.a $(TOP)/lib/crt0.o $(LIBC_OBJS_C)

QEMU=qemu-system-i386
