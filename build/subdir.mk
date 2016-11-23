all:: $(subdir)/makeheaders

includes:: $(subdir)/makeheaders

$(subdir)/makeheaders: $(subdir)/makeheaders.c
	$(HOSTCC) -g -o $@ $<

clean::
	$(RM) $(subdir)/makeheaders
