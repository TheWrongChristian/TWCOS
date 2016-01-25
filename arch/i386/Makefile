include ../../build/tools.mk

SRCS_S=asm.s setjmp.s
SRCS_C=console.c multiboot.c init.c vmap.c i386.c pci.c
HDRS_C=$(SRCS_C:.c=.h)

includes:: $(HDRS_C) arch.h
	$(MAKEHEADERS) $(SRCS_C)

arch.h:
	$(MAKEHEADERS) -h $(SRCS_C) > arch.h
	$(CP) arch.h include/arch/arch.h

all:: arch.h libarch.a 
	$(CP) libarch.a $(TOP)/lib

libarch.a: $(OBJS)
	$(AR) qs libarch.a $(OBJS)

$(HDRS_C) arch.h: $(SRCS_C)

clean::
	$(RM) libarch.a $(OBJS)
