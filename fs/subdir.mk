fsdir:=$(subdir)

subdir:=$(fsdir)/tarfs
include $(subdir)/subdir.mk
subdir:=$(fsdir)/devfs
include $(subdir)/subdir.mk
