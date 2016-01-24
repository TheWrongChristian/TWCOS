include ../build/tools.mk

SRCS_C=stream.c
HDRS=$(SRCS_C:.c=.h)

all:: libk.a libk.h
	$(CP) libk.a $(TOP)/lib

includes:: $(SRCS_C) libk.h
	$(MAKEHEADERS) $(SRCS_C)

libk.h: $(SRCS_C)
	$(MAKEHEADERS) -h $(SRCS_C) > libk.h
	mkdir -p $(TOP)/include/libk
	$(CP) libk.h $(TOP)/include/libk



libk.a: $(OBJS)
	$(AR) qs libk.a $(OBJS)

clean::
	$(RM) libk.a $(OBJS)
