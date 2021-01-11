TCL_SRCS_C := $(subdir)/tcl_test.c
TCL_OBJS_C := $(TCL_SRCS_C:.c=.o)

TCL := $(subdir)/tcl
INITRD_SBIN += $(TCL)

user:: $(TCL)

includes::
	$(MAKEHEADERS) $(TCL_SRCS_C)

$(TCL): $(USERLIBS) $(TCL_OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(TCL_OBJS_C)

clean::
	$(RM) $(TCL) $(TCL_OBJS_C)
