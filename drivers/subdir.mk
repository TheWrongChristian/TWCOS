drivers:=$(subdir)

subdir:=$(drivers)/char
include $(subdir)/subdir.mk
subdir:=$(drivers)/block
include $(subdir)/subdir.mk
subdir:=$(drivers)/net
include $(subdir)/subdir.mk
subdir:=$(drivers)/usb
include $(subdir)/subdir.mk
