ARCH_SRCS_S := $(subdir)/asm.S $(subdir)/setjmp.S
ARCH_SRCS_C := $(subdir)/console.c $(subdir)/multiboot.c $(subdir)/init.c $(subdir)/vmap.c $(subdir)/i386.c $(subdir)/pci.c
SRCS_S += $(ARCH_SRCS_S)
SRCS_C += $(ARCH_SRCS_C)
