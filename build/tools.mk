ifndef TOP
TOP=..
endif

TOOLS=$(TOP)/tools
HOSTCC=gcc
CROSS=i686-elf-
CC=ccache $(TOOLS)/bin/$(CROSS)gcc
CXX=ccache $(TOOLS)/bin/$(CROSS)g++
LD=$(TOOLS)/bin/$(CROSS)ld
AS=$(TOOLS)/bin/$(CROSS)as
AR=$(TOOLS)/bin/$(CROSS)ar
CP=cp -f
MAKEHEADERS=$(TOP)/build/makeheaders

include $(TOP)/build/param.mk

COPTS=-g -DDEBUG
# Override COPTS if present
-include $(TOP)/build/copts.mk

debug-config:
	echo 'COPTS=-g -DDEBUG' > $(TOP)/build/copts.mk

prod-config:
	echo 'COPTS=-O3' > $(TOP)/build/copts.mk

KOPTS:=-ffreestanding
UOPTS:=-I$(TOP)/user/pdclib/include -I$(TOP)/user/pdclib/platform/twcos/include
CFLAGS=$(COPTS) $(UOPTS) --sysroot=. -pipe -std=gnu99 -Wall -I$(TOP)/arch/$(ARCH)/include -I$(TOP)/include
CXXFLAGS=$(COPTS) $(UOPTS) --sysroot=. -pipe -Wall -I$(TOP)/arch/$(ARCH)/include -I$(TOP)/include
ASFLAGS=-g

OBJS=$(SRCS_S:.S=.o) $(SRCS_C:.c=.o)

$(OBJS): CFLAGS+=$(KOPTS)

%.d: %.c %.h
	-$(CC) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) -MG $<
%.d: %.S
	-$(CC) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) -MG $<

.PHONY: clean debug-config prod-config
