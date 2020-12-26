SH_SRCS_C := $(subdir)/main.c $(subdir)/tokenizer.c # $(subdir)/window.c $(subdir)/testshell.c
SH_OBJS_C := $(SH_SRCS_C:.c=.o)

SH2_SRCS_C := $(subdir)/shell.c
SH2_OBJS_C := $(SH2_SRCS_C:.c=.o)

SH := $(subdir)/sh
SH2 := $(subdir)/sh2
INITRD_SBIN += $(SH) $(SH2)

user:: $(SH) $(SH2)

includes::
	$(MAKEHEADERS) $(SH_SRCS_C)

$(SH): $(USERLIBS) $(SH_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(SH_OBJS_C)

$(SH2): $(USERLIBS) $(SH2_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(SH2_OBJS_C)

clean::
	$(RM) $(SH) $(SH2) $(SH_OBJS_C)
