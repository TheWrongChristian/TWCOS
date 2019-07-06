userdir:=$(subdir)

# Clear KOPTS - We don't want a freestanding object
KOPTS:=
UOPTS += --sysroot=.
UOPTS += -I$(TOP)/$(subdir)/pdclib/include
UOPTS += -I$(TOP)/$(subdir)/pdclib/platform/twcos/include

all:: $(TOP)/lib/libc.a $(TOP)/lib/libg.a $(TOP)/lib/crt0.o

subdir:=$(userdir)/pdclib
include $(subdir)/subdir.mk
subdir:=$(userdir)/shell
include $(subdir)/subdir.mk
