SRCS_C += $(subdir)/tarfs.c $(subdir)/tarfs.tar.c

TARFS_FILES := $(SRCS_KERNEL_C)
TARFS_TAR := $(subdir)/tarfs.tar
TARFS_TAR_C := $(subdir)/tarfs.tar.c

$(TARFS_TAR): $(TARFS_FILES)
	tar -cf $(TARFS_TAR) $(TARFS_FILES)

$(TARFS_TAR_C): $(TARFS_TAR)
	xxd -i $(TARFS_TAR) > $(TARFS_TAR_C)

includes:: $(TARFS_TAR_C)
