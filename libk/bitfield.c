#include "bitfield.h"

#if INTERFACE
#include <stdint.h>

#endif

uint32_t bitset(uint32_t flags, int start, int len, int value)
{
	uint32_t mask = ~(~0 << len);
	flags &= ~(mask << (1+start - len));
	flags |= (value << (1+start - len));

	return flags;
}

uint32_t bitget(uint32_t flags, int start, int len)
{
	uint32_t mask = ~(~0 << len);

	return (flags>>(start-len+1)) & mask;
}
