ifndef TOP
TOP=..
endif

TOOLS=$(TOP)/tools/bin
HOSTCC=gcc
CC=$(TOOLS)/i386-linux-gcc
LD=$(TOOLS)/i386-linux-ld
AS=$(TOOLS)/i386-linux-as -g
CP=cp -f
MAKEHEADERS=$(TOP)/build/makeheaders

include $(TOP)/build/param.mk

COPTS=-g
CFLAGS=$(COPTS) -std=gnu99 -ffreestanding -Wall -I$(TOP)/arch/$(ARCH)/include -I$(TOP)/include

OBJS=$(SRCS_S:.S=.o) $(SRCS_C:.c=.o)

%.d: %.c
	$(CC) $(COPTS) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) $<

%.d: %.S
	$(CC) $(COPTS) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) $<

.PHONY: clean
