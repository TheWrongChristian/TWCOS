GCC_VERSION=9.3.0
BINUTILS_VERSION=2.34
MIRROR=http://www.mirrorservice.org/sites/sourceware.org/pub/
GCC_DOWNLOAD=$(MIRROR)/gcc/releases/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz
BINUTILS_DOWNLOAD=$(MIRROR)/binutils/releases/binutils-$(BINUTILS_VERSION).tar.xz

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
	( cd binutils-$(BINUTILS_VERSION) && ./configure --prefix=$(TOOLS) --target=$(TARGET) && make all install )

cross-configure-gcc:
	( cd gcc-$(GCC_VERSION) && ./configure --prefix=$(TOOLS) --target=$(TARGET) --enable-languages=c --without-headers && make all install )

cross-configure: cross-configure-binutils cross-configure-gcc

cross-build-binutils:
	( cd binutils-$(BINUTILS_VERSION) && make all install )

cross-build-gcc:
	( cd binutils-$(GCC_VERSION) && make all install )

cross-build: cross-build-binutils cross-build-gcc
