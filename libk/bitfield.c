#include "bitfield.h"

#if INTERFACE
#include <stdint.h>

#endif

uint32_t bitset(uint32_t flags, int start, int len, int value)
{
	uint32_t mask = ~(~0 << len);
	flags &= ~(mask << (start - len));
	flags |= (value << (start - len));

	return flags;
}
