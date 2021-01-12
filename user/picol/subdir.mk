PICOL_SRCS_C := $(subdir)/picol.c $(subdir)/strdup.c
PICOL_OBJS_C := $(PICOL_SRCS_C:.c=.o)

PICOL=$(subdir)/picol
INITRD_SBIN += $(PICOL)

user:: $(PICOL)

$(PICOL): $(USERLIBS) $(PICOL_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(PICOL_OBJS_C)

clean::
	$(RM) $(PICOL) $(PICOL_OBJS_C)
