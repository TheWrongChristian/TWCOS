include tools.mk

all:: makeheaders

includes:: makeheaders

makeheaders: makeheaders.c
	$(HOSTCC) -g -o $@ $<

clean::
	$(RM) makeheaders
