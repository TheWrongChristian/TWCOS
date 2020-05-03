SRCS_DRIVERS_CHAR_C := $(subdir)/terminal.c $(subdir)/ns16550.c $(subdir)/framebuffer.c $(subdir)/font.c
FONT := $(subdir)/font

$(FONT).c: $(FONT).psf
	xxd -i $(FONT).psf $(FONT).c
