ARCH_SRCS_S := $(subdir)/asm.S $(subdir)/setjmp.S
ARCH_SRCS_C := $(subdir)/console.c $(subdir)/multiboot.c $(subdir)/init.c $(subdir)/vmap.c $(subdir)/i386.c $(subdir)/pci.c $(subdir)/isa.c $(subdir)/syscall.c
ARCH_SYSCALL_SH := $(subdir)/syscall.sh
ARCH_USYSCALL_SH := $(subdir)/usyscall.sh
ARCH_SYSCALL_LIST := $(subdir)/syscall.list
ARCH_SYSCALL_C := $(subdir)/syscall.c
ARCH_USYSCALL_C := $(subdir)/usyscall.c
ARCH_CRT_S := $(subdir)/crt0.S $(subdir)/crt1.S $(subdir)/crti.S $(subdir)/crtn.S
ARCH_CRT_O := $(ARCH_CRT_S:.S=.o)
ARCH_CRT0_S := $(subdir)/crt0.S
ARCH_CRT0_O := $(ARCH_CRT0_S:.S=.o)
ARCH_CRT1_S := $(subdir)/crt1.S
ARCH_CRT1_O := $(ARCH_CRT1_S:.S=.o)
ARCH_CRTi_S := $(subdir)/crti.S
ARCH_CRTi_O := $(ARCH_CRTi_S:.S=.o)
ARCH_CRTn_S := $(subdir)/crtn.S
ARCH_CRTn_O := $(ARCH_CRTn_S:.S=.o)
SRCS_S += $(ARCH_SRCS_S)
SRCS_C += $(ARCH_SRCS_C)

$(ARCH_SYSCALL_C): $(BUILD_SYSCALL_SH) $(ARCH_SYSCALL_SH) $(ARCH_SYSCALL_LIST)
	bash $(BUILD_SYSCALL_SH) $(ARCH_SYSCALL_SH) < $(ARCH_SYSCALL_LIST) > $(ARCH_SYSCALL_C)

$(ARCH_USYSCALL_C): $(BUILD_SYSCALL_SH) $(ARCH_USYSCALL_SH) $(ARCH_SYSCALL_LIST)
	bash $(BUILD_SYSCALL_SH) $(ARCH_USYSCALL_SH) < $(ARCH_SYSCALL_LIST) > $(ARCH_USYSCALL_C)

$(subdir)/asm.S: $(subdir)/isr.inc

$(subdir)/isr.inc: $(subdir)/isr.sh
	sh $< > $@

LD_SCRIPT := $(subdir)/linker.ld

$(subdir)/kernel: $(OBJS) $(LD_SCRIPT)
	$(CC) -T $(LD_SCRIPT) -o $@ -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

# all:: $(TOP)/lib/libsyscall.a $(TOP)/lib/crt0.o
# all:: $(ARCH_CRT_O)
#	$(CP) $(ARCH_CRT_O) $(TOP)/lib

USERLIBS += $(TOP)/lib/crt0.o $(TOP)/lib/crt1.o $(TOP)/lib/crti.o $(TOP)/lib/crtn.o
all:: $(USERLIBS)

# $(TOP)/lib/libsyscall.a: $(USYSCALL_SRCS_O)
# 	$(AR) rcs $@ $(USYSCALL_SRCS_O)

$(TOP)/lib/crt0.o: $(ARCH_CRT0_O)
	$(CP) $< $@

$(TOP)/lib/crt1.o: $(ARCH_CRT1_O)
	$(CP) $< $@

$(TOP)/lib/crtn.o: $(ARCH_CRTn_O)
	$(CP) $< $@

$(TOP)/lib/crti.o: $(ARCH_CRTi_O)
	$(CP) $< $@

clean::
	$(RM) $(TOP)/lib/libsyscall.a $(TOP)/lib/crt0.o $(LIBC_OBJS_C) $(subdir)/isr.inc $(ARCH_SYSCALL_C) $(ARCH_USYSCALL_C)

QEMU=qemu-system-i386
TARGET=i686-elf
CROSS=$(TARGET)-
