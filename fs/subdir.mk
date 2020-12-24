fsdir:=$(subdir)

subdir:=$(fsdir)/tarfs
include $(subdir)/subdir.mk
subdir:=$(fsdir)/devfs
include $(subdir)/subdir.mk
subdir:=$(fsdir)/fatfs
include $(subdir)/subdir.mk
subdir:=$(fsdir)/procfs
include $(subdir)/subdir.mk
