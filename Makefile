all::

.PHONY: userlibs
.PHONY: clean
.PHONY: all

TOP=$(CURDIR)

ARCH=i386

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
subdir := build
include $(subdir)/tools.mk
subdir := arch/$(ARCH)
include $(subdir)/subdir.mk
subdir := user
include $(subdir)/subdir.mk
subdir := initrd
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

clean::
	rm -rf $(OBJS) $(SRCS_C:.c=.h) boot.iso

.gdbinit:
	echo target remote localhost:1234 | tee .gdbinit
	echo symbol-file $(KERNEL) | tee -a .gdbinit
	echo break kernel_main | tee -a .gdbinit

QEMU_OPTS=-d cpu_reset,guest_errors -serial stdio
QEMU_MEM=8m
qemu: all .gdbinit
	$(QEMU) $(QEMU_OPTS) -m $(QEMU_MEM) -s -S -kernel $(KERNEL) -initrd $(INITRD_TAR)

run: all
	$(QEMU) $(QEMU_OPTS) -m $(QEMU_MEM) -s -kernel $(KERNEL) -initrd $(INITRD_TAR)

system: all
	$(QEMU) $(QEMU_OPTS) -m $(QEMU_MEM) -s -cdrom boot.iso

system-qemu: all
	$(QEMU) $(QEMU_OPTS) -m $(QEMU_MEM) -s -S -cdrom boot.iso

run-gcoverhead: all
	$(QEMU) -enable-kvm $(QEMU_OPTS) -m $(QEMU_MEM) -kernel $(KERNEL) -initrd $(INITRD_TAR) &
	$(QEMU) -enable-kvm $(QEMU_OPTS) -m 10$(QEMU_MEM) -kernel $(KERNEL) -initrd $(INITRD_TAR)

qemu-kvm: all .gdbinit
	$(QEMU) -enable-kvm $(QEMU_OPTS) -m $(QEMU_MEM) -s -S -kernel $(KERNEL) -initrd $(INITRD_TAR)

run-kvm: all
	$(QEMU) -enable-kvm $(QEMU_OPTS) -m $(QEMU_MEM) -s -kernel $(KERNEL) -initrd $(INITRD_TAR)

includes::
	mkdir -p lib
	$(MAKEHEADERS) $(SRCS_C) $(ARCH_USYSCALL_C) $(PDCLIB_TWCOS_SRCS_C)

cflow:
	cflow -d 4 -m kernel_main $(SRCS_C) $(LIBC_SRCS_C) $(INIT_SRCS_C)

cflowr:
	cflow -d 4 -r $(SRCS_C) $(LIBC_SRCS_C) $(INIT_SRCS_C)

cxref:
	cxref -html-src $(SRCS_C) $(SRCS_C:.c=.h)

ctags:
	ctags $(SRCS_C) $(LIBC_SRCS_C) $(INIT_SRCS_C)

cppcheck:
	cppcheck $(SRCS_C)

cppcheck-gui:
	cppcheck-gui $(SRCS_C)

-include $(OBJS:.o=.d)

clean::
	rm -f $(OBJS:.o=.d)
