INIT_SRCS_C := $(subdir)/main.c
INIT_OBJS_C := $(INIT_SRCS_C:.c=.o)

INIT=$(subdir)/init

all:: $(INIT)

$(INIT): $(INIT_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(INIT_OBJS_C)
