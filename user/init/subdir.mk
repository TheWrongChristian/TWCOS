INIT_SRCS_C := $(subdir)/init.c
INIT_OBJS_C := $(INIT_SRCS_C:.c=.o)

INIT := $(subdir)/init
INITRD_SBIN += $(INIT)

user:: $(INIT)

includes::
	$(MAKEHEADERS) $(INIT_SRCS_C)

$(INIT): $(USERLIBS) $(INIT_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(INIT_OBJS_C)

clean::
	$(RM) $(INIT) $(INIT_OBJS_C)
