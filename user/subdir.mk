userdir:=$(subdir)

subdir:=$(userdir)/pdclib
include $(subdir)/subdir.mk
subdir:=$(userdir)/shell
include $(subdir)/subdir.mk
subdir:=$(userdir)/picol
include $(subdir)/subdir.mk
