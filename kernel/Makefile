include ../build/tools.mk

SRCS_C=main.c core.c pci.c printk.c panic.c

all:: kernel

.PHONY: kernel
kernel: $(OBJS)
	$(CC) -T linker.ld -o kernel -ffreestanding -O2 -nostdlib $(OBJS) -L$(TOP)/lib -lk -larch -lgcc

kernel.h:
	$(MAKEHEADERS) -h $(SRCS_C) $(TOP)/arch/$(ARCH)/include/arch/arch.h > kernel.h

clean::
	rm -rf isodir
	rm -f $(OBJS) kernel boot.iso

includes:: kernel.h
	$(MAKEHEADERS) $(SRCS_C)
	mkdir -p $(TOP)/include/kernel
	$(CP) kernel.h $(TOP)/include/kernel/kernel.h
