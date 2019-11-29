userdir:=$(subdir)

# Clear KOPTS - We don't want a freestanding object
KOPTS:=
UOPTS += --sysroot=.
UOPTS += -I$(TOP)/$(subdir)/pdclib/include
UOPTS += -I$(TOP)/$(subdir)/pdclib/platform/twcos/include

subdir:=$(userdir)/pdclib
include $(subdir)/subdir.mk
subdir:=$(userdir)/shell
include $(subdir)/subdir.mk
subdir:=$(userdir)/picol
include $(subdir)/subdir.mk
