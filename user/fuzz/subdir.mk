FUZZ_SRCS_C := $(subdir)/fuzz.c
FUZZ_OBJS_C := $(FUZZ_SRCS_C:.c=.o)

FUZZ := $(subdir)/fuzz
INITRD_SBIN += $(FUZZ)

user:: $(FUZZ)

includes::
	$(MAKEHEADERS) $(FUZZ_SRCS_C)

$(FUZZ): $(USERLIBS) $(FUZZ_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(FUZZ_OBJS_C)

clean::
	$(RM) $(FUZZ) $(FUZZ_OBJS_C)
