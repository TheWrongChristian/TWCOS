#include <stdint.h>
#include "bits.h"

#if INTERFACE

#define	__BIT(n)	(1 << (n))
#define __BITS(hi,lo)	((~((~0)<<((hi)+1)))&((~0)<<(lo)))

#define __LOWEST_SET_BIT(__mask) ((((__mask) - 1) & (__mask)) ^ (__mask))
#define __SHIFTOUT(__x, __mask) (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))
#define __SHIFTIN(__x, __mask) ((__x) * __LOWEST_SET_BIT(__mask))

#endif

void bits_rmw32(volatile uint32_t * p, int hi, int lo, uint32_t field)
{
	const uint32_t mask = __BITS(hi, lo);
	uint32_t val = *p;

	val &= ~mask;
	val |= __SHIFTIN(field, mask);
	*p = val;
}

void bits_test()
{
	uint32_t v = 0;

	bits_rmw32(&v, 12, 10, 5);
}
