PDCLIB_SRCS_C := \
  $(subdir)/functions/_PDCLIB/_PDCLIB_print.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_time.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_digits.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_closeall.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_messages.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_filemode.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lines.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_prepread.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_strtox_prelim.c\
  $(subdir)/functions/_PDCLIB/stdarg.c\
  $(subdir)/functions/_PDCLIB/assert.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_scan.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_atomax.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_strtox_main.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_collate.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_seed.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_ctype.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_monetary.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_load_lc_numeric.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_is_leap.c\
  $(subdir)/functions/_PDCLIB/errno.c\
  $(subdir)/functions/_PDCLIB/_PDCLIB_prepwrite.c\
  $(subdir)/functions/ctype/isalpha.c\
  $(subdir)/functions/ctype/isxdigit.c\
  $(subdir)/functions/ctype/isupper.c\
  $(subdir)/functions/ctype/ispunct.c\
  $(subdir)/functions/ctype/isgraph.c\
  $(subdir)/functions/ctype/isdigit.c\
  $(subdir)/functions/ctype/toupper.c\
  $(subdir)/functions/ctype/isblank.c\
  $(subdir)/functions/ctype/isprint.c\
  $(subdir)/functions/ctype/tolower.c\
  $(subdir)/functions/ctype/iscntrl.c\
  $(subdir)/functions/ctype/islower.c\
  $(subdir)/functions/ctype/isalnum.c\
  $(subdir)/functions/ctype/isspace.c\
  $(subdir)/functions/inttypes/strtoimax.c\
  $(subdir)/functions/inttypes/strtoumax.c\
  $(subdir)/functions/inttypes/imaxdiv.c\
  $(subdir)/functions/inttypes/imaxabs.c\
  $(subdir)/functions/time/difftime.c\
  $(subdir)/functions/time/localtime.c\
  $(subdir)/functions/time/gmtime.c\
  $(subdir)/functions/time/strftime.c\
  $(subdir)/functions/time/asctime.c\
  $(subdir)/functions/time/ctime.c\
  $(subdir)/functions/time/mktime.c\
  $(subdir)/functions/locale/localeconv.c\
  $(subdir)/functions/locale/setlocale.c\
  $(subdir)/functions/stdlib/atol.c\
  $(subdir)/functions/stdlib/atexit.c\
  $(subdir)/functions/stdlib/strtoll.c\
  $(subdir)/functions/stdlib/free.c\
  $(subdir)/functions/stdlib/abort.c\
  $(subdir)/functions/stdlib/realloc.c\
  $(subdir)/functions/stdlib/srand.c\
  $(subdir)/functions/stdlib/ldiv.c\
  $(subdir)/functions/stdlib/strtoul.c\
  $(subdir)/functions/stdlib/llabs.c\
  $(subdir)/functions/stdlib/strtoull.c\
  $(subdir)/functions/stdlib/bsearch.c\
  $(subdir)/functions/stdlib/malloc.c\
  $(subdir)/functions/stdlib/strtol.c\
  $(subdir)/functions/stdlib/atoll.c\
  $(subdir)/functions/stdlib/_Exit.c\
  $(subdir)/functions/stdlib/lldiv.c\
  $(subdir)/functions/stdlib/exit.c\
  $(subdir)/functions/stdlib/rand.c\
  $(subdir)/functions/stdlib/abs.c\
  $(subdir)/functions/stdlib/qsort.c\
  $(subdir)/functions/stdlib/atoi.c\
  $(subdir)/functions/stdlib/calloc.c\
  $(subdir)/functions/stdlib/div.c\
  $(subdir)/functions/stdlib/labs.c\
  $(subdir)/functions/string/strspn.c\
  $(subdir)/functions/string/strxfrm.c\
  $(subdir)/functions/string/memcpy.c\
  $(subdir)/functions/string/memset.c\
  $(subdir)/functions/string/memchr.c\
  $(subdir)/functions/string/strcat.c\
  $(subdir)/functions/string/strtok.c\
  $(subdir)/functions/string/memmove.c\
  $(subdir)/functions/string/strcmp.c\
  $(subdir)/functions/string/strncpy.c\
  $(subdir)/functions/string/strrchr.c\
  $(subdir)/functions/string/strlen.c\
  $(subdir)/functions/string/strstr.c\
  $(subdir)/functions/string/strcoll.c\
  $(subdir)/functions/string/strcspn.c\
  $(subdir)/functions/string/strerror.c\
  $(subdir)/functions/string/memcmp.c\
  $(subdir)/functions/string/strpbrk.c\
  $(subdir)/functions/string/strncmp.c\
  $(subdir)/functions/string/strcpy.c\
  $(subdir)/functions/string/strncat.c\
  $(subdir)/functions/string/strchr.c\
  $(subdir)/functions/stdio/fputc.c\
  $(subdir)/functions/stdio/vsnprintf.c\
  $(subdir)/functions/stdio/ferror.c\
  $(subdir)/functions/stdio/fread.c\
  $(subdir)/functions/stdio/puts.c\
  $(subdir)/functions/stdio/tmpnam.c\
  $(subdir)/functions/stdio/fwrite.c\
  $(subdir)/functions/stdio/setbuf.c\
  $(subdir)/functions/stdio/feof.c\
  $(subdir)/functions/stdio/clearerr.c\
  $(subdir)/functions/stdio/rename.c\
  $(subdir)/functions/stdio/perror.c\
  $(subdir)/functions/stdio/getchar.c\
  $(subdir)/functions/stdio/scanf.c\
  $(subdir)/functions/stdio/printf.c\
  $(subdir)/functions/stdio/sprintf.c\
  $(subdir)/functions/stdio/rewind.c\
  $(subdir)/functions/stdio/vprintf.c\
  $(subdir)/functions/stdio/vsprintf.c\
  $(subdir)/functions/stdio/ftell.c\
  $(subdir)/functions/stdio/fputs.c\
  $(subdir)/functions/stdio/fseek.c\
  $(subdir)/functions/stdio/putc.c\
  $(subdir)/functions/stdio/vsscanf.c\
  $(subdir)/functions/stdio/putchar.c\
  $(subdir)/functions/stdio/setvbuf.c\
  $(subdir)/functions/stdio/fflush.c\
  $(subdir)/functions/stdio/vfscanf.c\
  $(subdir)/functions/stdio/sscanf.c\
  $(subdir)/functions/stdio/fclose.c\
  $(subdir)/functions/stdio/ungetc.c\
  $(subdir)/functions/stdio/vfprintf.c\
  $(subdir)/functions/stdio/fgetpos.c\
  $(subdir)/functions/stdio/fsetpos.c\
  $(subdir)/functions/stdio/fprintf.c\
  $(subdir)/functions/stdio/snprintf.c\
  $(subdir)/functions/stdio/vscanf.c\
  $(subdir)/functions/stdio/fscanf.c\
  $(subdir)/functions/stdio/fgetc.c\
  $(subdir)/functions/stdio/getc.c\
  $(subdir)/functions/stdio/freopen.c\
  $(subdir)/functions/stdio/fopen.c\
  $(subdir)/functions/stdio/fgets.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_fillbuffer.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_Exit.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_open.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_stdinit.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_flushbuffer.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_seek.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_allocpages.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_close.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_rename.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_cstart.c\
  $(subdir)/platform/twcos/functions/_PDCLIB/_PDCLIB_cppstart.c\
  $(subdir)/platform/twcos/functions/signal/raise.c\
  $(subdir)/platform/twcos/functions/signal/signal.c\
  $(subdir)/platform/twcos/functions/time/timespec_get.c\
  $(subdir)/platform/twcos/functions/time/clock.c\
  $(subdir)/platform/twcos/functions/time/time.c\
  $(subdir)/platform/twcos/functions/stdlib/getenv.c\
  $(subdir)/platform/twcos/functions/stdlib/system.c\
  $(subdir)/platform/twcos/functions/stdio/remove.c\
  $(subdir)/platform/twcos/functions/stdio/tmpfile.c

PDCLIB_TWCOS_SRCS_C := \
  $(subdir)/platform/twcos/syscalls/syscall.c

LIBC_SRCS_C += $(PDCLIB_SRCS_C) $(PDCLIB_TWCOS_SRCS_C) $(ARCH_USYSCALL_C)
LIBC_OBJS_C = $(LIBC_SRCS_C:.c=.o)

USERLIBS += $(TOP)/lib/libc.a $(TOP)/lib/libg.a

$(TOP)/lib/libc.a: $(LIBC_OBJS_C)
	mkdir -p $(TOP)/lib
	$(AR) rcs $@ $(LIBC_OBJS_C)

$(TOP)/lib/libg.a: $(subdir)/dummy.o
	$(AR) rcs $@ $<

clean::
	$(RM) $(TOP)/lib/libc.a $(TOP)/lib/libg.a $(LIBC_OBJS_C)

