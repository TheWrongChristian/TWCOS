SRCS_C += $(subdir)/main.c  $(subdir)/core.c  $(subdir)/pci.c  $(subdir)/printk.c  $(subdir)/panic.c

kernel: $(subdir)/kernel

$(subdir)/kernel: $(OBJS)
	$(CC) -T $(subdir)/linker.ld -o $@ -ffreestanding -O2 -nostdlib $(OBJS) -lgcc
