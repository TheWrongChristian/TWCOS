#include "utf8.h"

#if INTERFACE

#include <stddef.h>
#include <stdint.h>

typedef uint16_t ucs16_t;

#endif


size_t utf8_from_ucs16(unsigned char * utf8, size_t utf8len, ucs16_t * ucs, size_t ucslen)
{
	unsigned char * p = utf8;
	size_t plen = 0;
	for(int i=0; i<ucslen && plen<utf8len; i++, ucs++) {
		ucs16_t u = *ucs;

		if (u>0 && u<0x80) {
			*p++ = u;
		} else if (u>=0x80 && u<0x800) {
			*p++ = 0xc0 | ((u >> 6) & 0x1f);
			*p++ = 0x80 | (u & 0x3f);
		} else if (u) {
			*p++ = 0xe0 | ((u >> 12) & 0xf);
			*p++ = 0x80 | ((u >> 6) & 0x3f);
			*p++ = 0x80 | (u & 0x3f);
		} else {
			*p++ = 0;
			return p-utf8;
		}
	}

	return p-utf8;
}

size_t utf8_to_ucs16(unsigned char * utf8, size_t utf8len, ucs16_t * ucs, size_t ucslen)
{
	ucs16_t * p = ucs;
	size_t plen = 0;
	for(int i=0; i<utf8len && plen<ucslen; i++, plen++) {
		unsigned char c = *utf8++;
		ucs16_t u = 0;

		if (c>0 && c<0x80) {
			u = c;
		} else if (c>=0xc0 && c<0xe0) {
			u = c & 0x1f;
			u <<= 6;
			u |= (*utf8++ & 0x3f);
		} else if (c) {
			u = c & 0xf;
			u <<= 6;
			u |= (*utf8++ & 0x3f);
			u <<= 6;
			u |= (*utf8++ & 0x3f);
		} else {
			*p++ = 0;
			return p-ucs;
		}

		*p++ = u;
	}

	return p-ucs;
}

void utf8_test()
{
	ucs16_t in[] = { 0x24, 0xa2, 0x939, 0x20ac, 0xd55c, 0};
	ucs16_t out[countof(in)];
	char utf8[16] = {0};
	utf8_from_ucs16(utf8, countof(utf8), in, countof(in));
	utf8_to_ucs16(utf8, countof(utf8), out, countof(out));
}
