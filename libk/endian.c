#include "endian.h"

#if INTERFACE

#include <stdint.h>

typedef uint32_t le32_t;
typedef uint16_t le16_t;
typedef uint32_t be32_t;
typedef uint16_t be16_t;

#endif

#ifndef LITTLE_ENDIAN
#error "No byte order defined"
#endif

static uint16_t bswap16(uint16_t v)
{
	return (v >> 8) | (v << 8);
}

static uint32_t bswap32(uint32_t v)
{
	return bswap16(v >> 16) | bswap16(v & 0xffff) << 16;
}

uint32_t le32(uint32_t v)
{
#if LITTLE_ENDIAN
	return v;
#else
	return bswap32(v);
#endif
}

uint16_t le16(uint16_t v)
{
#if LITTLE_ENDIAN
	return v;
#else
	return bswap32(v);
#endif
}

uint32_t be32(uint32_t v)
{
#if LITTLE_ENDIAN
	return bswap32(v);
#else
	return v;
#endif
}

uint16_t be16(uint16_t v)
{
#if LITTLE_ENDIAN
	return bswap16(v);
#else
	return v;
#endif
}
