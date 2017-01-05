ifndef TOP
TOP=..
endif

TOOLS=$(TOP)/tools/bin
HOSTCC=gcc
CC=$(TOOLS)/i386-linux-gcc
LD=$(TOOLS)/i386-linux-ld
AS=$(TOOLS)/i386-linux-as
CP=cp -f
MAKEHEADERS=$(TOP)/build/makeheaders

include $(TOP)/build/param.mk

COPTS=-g
CFLAGS=$(COPTS) -std=gnu99 -ffreestanding -Wall -I$(TOP)/arch/$(ARCH)/include -I$(TOP)/include
ASFLAGS=-g

OBJS=$(SRCS_S:.S=.o) $(SRCS_C:.c=.o)

%.d: %.c %.h
	$(CC) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) -MG $<
%.d: %.S
	$(CC) $(CFLAGS) -M -MF $@ -MT $(@:.d=.o) -MG $<

.PHONY: clean
