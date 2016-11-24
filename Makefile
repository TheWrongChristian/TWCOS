TOP=$(CURDIR)

ARCH=i386

all::

OBJS=$(SRCS_S:.S=.o) $(SRCS_C:.c=.o)
SRCS_C :=
SRCS_S :=

subdir := build
include $(subdir)/subdir.mk
subdir := arch/$(ARCH)
include $(subdir)/subdir.mk
subdir := libk
include $(subdir)/subdir.mk
subdir := kernel
include $(subdir)/subdir.mk

include $(TOP)/build/tools.mk

all:: lib boot.iso

obj:
	mkdir -p obj

.PHONY: kernel
boot.iso: grub.cfg kernel
	mkdir -p isodir/boot/grub
	cp kernel/kernel isodir/boot/kernel
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o boot.iso isodir

.PHONY: clean
clean::
	rm -rf $(OBJS) $(SRCS_C:.c=.h) boot.iso

gdb: all
	echo target remote localhost:1234 | tee .gdbinit
	echo symbol-file kernel/kernel | tee -a .gdbinit
	qemu-system-i386 -m 16 -s -S -kernel kernel/kernel & gdbtui

includes::
	rm -f $(SRCS_C:.c=.h)
	$(MAKEHEADERS) $(SRCS_C)

-include $(OBJS:.o=.d)

clean::
	rm -f $(OBJS:.o=.d)
