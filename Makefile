TOP=$(CURDIR)

ARCH=i386

SUBDIRS=build arch/$(ARCH) libk kernel


include $(TOP)/build/tools.mk

all:: lib boot.iso

lib:
	mkdir -p lib

.PHONY: kernel
kernel: build arch/$(ARCH) libk

boot.iso: grub.cfg kernel
	mkdir -p isodir/boot/grub
	cp kernel/kernel isodir/boot/kernel
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o boot.iso isodir

.PHONY: clean
clean::
	rm -rf lib/*

gdb: all
	qemu-system-i386 -s -S -kernel kernel/kernel &
	echo target remote localhost:1234 | tee .gdbinit
	echo symbol-file kernel/kernel | tee -a .gdbinit
	gdbtui
