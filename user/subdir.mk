userdir:=$(subdir)

INITRD_SBIN:=

subdir:=$(userdir)/pdclib
include $(subdir)/subdir.mk
subdir:=$(userdir)/init
include $(subdir)/subdir.mk
subdir:=$(userdir)/shell
include $(subdir)/subdir.mk
subdir:=$(userdir)/picol
include $(subdir)/subdir.mk
subdir:=$(userdir)/partcl
include $(subdir)/subdir.mk
