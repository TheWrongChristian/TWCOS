SRCS_C += $(subdir)/main.c  $(subdir)/core.c  $(subdir)/pci.c  $(subdir)/printk.c  $(subdir)/panic.c $(subdir)/thread.c $(subdir)/check.c

kernel: $(subdir)/kernel

LD_SCRIPT := $(subdir)/linker.ld

$(subdir)/kernel: $(OBJS) $(LD_SCRIPT)
	$(CC) -T $(LD_SCRIPT) -o $@ -ffreestanding -O2 -nostdlib $(OBJS) -lgcc
