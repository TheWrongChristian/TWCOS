TOP=$(CURDIR)

ARCH=i386

all::

OBJS=$(SRCS_S:.S=.o) $(SRCS_C:.c=.o)
SRCS_C :=
SRCS_S :=

subdir := build
include $(subdir)/subdir.mk
subdir := libk
include $(subdir)/subdir.mk
subdir := kernel
include $(subdir)/subdir.mk
subdir := fs
include $(subdir)/subdir.mk
subdir := drivers
include $(subdir)/subdir.mk
subdir := shell
include $(subdir)/subdir.mk
subdir := build
include $(subdir)/tools.mk
subdir := arch/$(ARCH)
include $(subdir)/subdir.mk
subdir := user
include $(subdir)/subdir.mk

all:: boot.iso

obj:
	mkdir -p obj

KERNEL=arch/$(ARCH)/kernel
boot.iso: grub.cfg $(KERNEL) $(INITRD_TAR)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/kernel
	cp $(INITRD_TAR) isodir/boot/initrd
	cp -f grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o boot.iso isodir

.PHONY: clean
clean::
	rm -rf $(OBJS) $(SRCS_C:.c=.h) boot.iso

.gdbinit:
	echo target remote localhost:1234 | tee .gdbinit
	echo symbol-file $(KERNEL) | tee -a .gdbinit
	echo break kernel_main | tee -a .gdbinit

qemu: all .gdbinit
	qemu-system-i386 -m 4 -s -S -kernel $(KERNEL) -initrd $(INITRD_TAR) &

run: all
	qemu-system-i386 -m 4 -s -kernel $(KERNEL) -initrd $(INITRD_TAR) &

includes::
	$(MAKEHEADERS) $(SRCS_C) $(ARCH_USYSCALL_C) $(PDCLIB_TWCOS_SRCS_C)

cflow:
	cflow -d 4 -m kernel_main $(SRCS_C)

cflowr:
	cflow -d 4 -r $(SRCS_C)

cxref:
	cxref -html-src $(SRCS_C) $(SRCS_C:.c=.h)

ctags:
	ctags $(SRCS_C)

cppcheck:
	cppcheck $(SRCS_C)

cppcheck-gui:
	cppcheck-gui $(SRCS_C)

-include $(OBJS:.o=.d)

clean::
	rm -f $(OBJS:.o=.d)
