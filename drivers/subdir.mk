drivers:=$(subdir)

subdir:=$(drivers)/char
include $(subdir)/subdir.mk
subdir:=$(drivers)/block
include $(subdir)/subdir.mk
subdir:=$(drivers)/net
include $(subdir)/subdir.mk
subdir:=$(drivers)/usb
include $(subdir)/subdir.mk

SRCS_C += $(SRCS_DRIVERS_CHAR_C) $(SRCS_DRIVERS_BLOCK_C) $(SRCS_DRIVERS_NET_C) $(SRCS_DRIVERS_USB_C)
