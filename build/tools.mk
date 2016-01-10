ifndef TOP
error TOP not defined
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

OBJS=$(SRCS_S:.s=.o) $(SRCS_C:.c=.o)

.PHONY: clean

.PHONY: all clean includes
ifdef SUBDIRS

all::
	for d in $(SUBDIRS); \
	do \
		$(MAKE) -C $$d TOP=$(TOP) all; \
	done

includes::
	for d in $(SUBDIRS); \
	do \
		$(MAKE) -C $$d TOP=$(TOP) includes; \
	done

clean::
	for d in $(SUBDIRS); \
	do \
		$(MAKE) -C $$d TOP=$(TOP) clean; \
	done
else
all::

includes::

clean::
endif
