SRCS_C += $(subdir)/tarfs.c $(subdir)/tarfs.tar.c

TARFS_FILES := $(SRCS_KERNEL_C) $(SRCS_LIBK_C)
TARFS_TAR := $(subdir)/tarfs.tar
TARFS_TAR_C := $(subdir)/tarfs.tar.c
TARFS_DEEPDIR := $(subdir)/a/deep/directory/

$(TARFS_DEEPDIR)/file1:
	mkdir -p $(TARFS_DEEPDIR)
	touch $@

$(TARFS_DEEPDIR)/file2:
	mkdir -p $(TARFS_DEEPDIR)
	touch $@

$(TARFS_TAR): $(TARFS_FILES) $(TARFS_DEEPDIR)/file1 $(TARFS_DEEPDIR)/file2
	tar -cf $(TARFS_TAR) $(TARFS_FILES) $(TARFS_DEEPDIR)

$(TARFS_TAR_C): $(TARFS_TAR)
	xxd -i $(TARFS_TAR) > $(TARFS_TAR_C)

includes:: $(TARFS_TAR_C)
