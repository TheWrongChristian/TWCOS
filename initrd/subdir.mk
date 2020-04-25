INITRD_TAR := $(subdir)/initrd.tar
INITRD_INITRD := $(subdir)/initrd
INITRD_INITRD_DEVFS = $(INITRD_INITRD)/devfs
INITRD_INITRD_SBIN = $(INITRD_INITRD)/sbin

$(INITRD_TAR):
	( cd $(INITRD_INITRD) && tar -cf - * ) > $(INITRD_TAR)

includes::
	mkdir -p $(INITRD_INITRD_SBIN) $(INITRD_INITRD_DEVFS)

clean::
	rm -rf $(INITRD_INITRD) $(TARFS_TAR)
