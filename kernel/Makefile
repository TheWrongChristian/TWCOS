include $(TOP)/build/tools.mk

SRCS_C=kernel.c core.c

all:: kernel

.PHONY: kernel
kernel: $(OBJS)
	$(CC) -T linker.ld -o kernel -ffreestanding -O2 -nostdlib $(OBJS) -L$(TOP)/lib -larch -lk -lgcc

clean::
	rm -rf isodir
	rm -f $(OBJS) kernel boot.iso

includes::
	$(MAKEHEADERS) $(SRCS_C)
