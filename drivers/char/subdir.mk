SRCS_DRIVERS_CHAR_C := $(subdir)/terminal.c $(subdir)/ns16550.c $(subdir)/framebuffer.c $(subdir)/font.c
FONT := $(subdir)/font

includes:: $(FONT).c

$(FONT).c: $(FONT).psf
	xxd -i $(FONT).psf $(FONT).c
SRCS_C += $(SRCS_DRIVERS_CHAR_C)
