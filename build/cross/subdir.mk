GCC_VERSION=9.3.0
BINUTILS_VERSION=2.34
MIRROR=http://www.mirrorservice.org/sites/sourceware.org/pub/
GCC_DOWNLOAD=$(MIRROR)/gcc/releases/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
BINUTILS_DOWNLOAD=$(MIRROR)/binutils/releases/binutils-$(BINUTILS_VERSION).tar.xz
GCC_BUILD=$(TOP)/gcc-build
BINUTILS_BUILD=$(TOP)/binutils-build
CONFIGURE_OPTIONS=--prefix=$(TOOLS) --target=$(TARGET) --disable-nls

cross:: unpack cross-configure cross-build

download:: gcc-$(GCC_VERSION).tar.xz binutils-$(BINUTILS_VERSION).tar.xz

gcc-$(GCC_VERSION).tar.xz:
	wget $(GCC_DOWNLOAD)

binutils-$(BINUTILS_VERSION).tar.xz:
	wget $(BINUTILS_DOWNLOAD)

unpack:: gcc-$(GCC_VERSION).tar.xz binutils-$(BINUTILS_VERSION).tar.xz
	tar xJf gcc-$(GCC_VERSION).tar.xz
	tar xJf binutils-$(BINUTILS_VERSION).tar.xz

cross-configure-binutils:
	mkdir -p $(BINUTILS_BUILD)
	( cd $(BINUTILS_BUILD) && ../binutils-$(BINUTILS_VERSION)/configure $(CONFIGURE_OPTIONS) --with-sysroot)

cross-configure-gcc:
	mkdir -p $(GCC_BUILD)
	( cd $(GCC_BUILD) && ../gcc-$(GCC_VERSION)/configure $(CONFIGURE_OPTIONS) --disable-libssp --enable-languages=c --without-headers --disable-libquadmath )

cross-configure: cross-configure-binutils cross-configure-gcc

cross-build-binutils:
	( cd $(BINUTILS_BUILD) && make all install )

cross-build-gcc:
	( cd $(GCC_BUILD) && make all install )

cross-build: cross-build-binutils cross-build-gcc