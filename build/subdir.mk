all:: $(subdir)/makeheaders

includes:: $(subdir)/makeheaders

$(subdir)/makeheaders: $(subdir)/makeheaders.c
	$(HOSTCC) -g -o $@ $<

clean::
	$(RM) $(subdir)/makeheaders

builddir:=$(subdir)

subdir:=$(builddir)/cross
include $(subdir)/subdir.mk

BUILD_SYSCALL_SH=$(builddir)/syscall.sh
