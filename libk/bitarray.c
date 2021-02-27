#include "bitarray.h"

#if INTERFACE
#include <stdint.h>

typedef uint32_t * bitarray;

#endif

int bitarray_firstset(bitarray array, int size)
{
	for(int i=0; i<size; i+=32) {
		uint32_t a = array[i/32];

		if (a) {
			int slot = i+31;
			if (a & 0xffff0000) slot -= 16, a >>= 16;
			if (a & 0xff00) slot -= 8, a >>= 8;
			if (a & 0xf0) slot -= 4, a >>= 4;
			if (a & 0xc) slot -= 2, a >>= 2;
			if (a & 0x2) slot -= 1, a >>= 1;

			return (slot<size) ? slot : -1;
		}
	}

	return -1;
}

int bitarray_get(bitarray array, int bit)
{
	uint32_t mask = 0x80000000 >> (bit & 0x1f);

	return 0 != (array[bit/32] & mask);
}

void bitarray_set(bitarray array, int bit, int value)
{
	uint32_t mask = 0x80000000 >> (bit & 0x1f);
	if (value) {
		array[bit/32] |= mask;
	} else {
		array[bit/32] &= ~mask;
	}
}

void bitarray_setall(bitarray array, int size, int value)
{
	uint32_t v = (value) ? 0xffffffff : 0;
	for(int i=0; i<size; i+=32) {
		array[i/32] = v;
	}
	if (size & 31) {
		v <<= (32-(size&31));
		array[size/32] = v;
	}
}

void bitarray_invert(bitarray array, int size)
{
	for(int i=0; i<size; i+=32) {
		array[i/32] ^= ~0;
	}
}

void bitarray_or(bitarray to, bitarray from, int size)
{
	for(int i=0; i<size; i+=32) {
		to[i/32] |= from[i/32];
	}
}

void bitarray_copy(bitarray to, bitarray from, int size)
{
	for(int i=0; i<size; i+=32) {
		to[i/32] = from[i/32];
	}
	if (size & 31) {
		uint32_t mask = ~0 << (32-(size&31));
		to[size/32] &= mask;
	}
}

void bitarray_test()
{
	uint32_t s[] = {0xab732672, 0xfe625331};
	uint32_t array[] = { 0, 0 };

	bitarray_copy(array, s, 45);

	bitarray_setall(array, 45, 1);
	bitarray_setall(array, 45, 0);

	bitarray_set(array, 10, 1);
	bitarray_set(array, 12, 1);
	bitarray_set(array, 42, 1);
	assert(10 == bitarray_firstset(array, sizeof(array) * 8));
	bitarray_set(array, 10, 0);
	assert(12 == bitarray_firstset(array, sizeof(array) * 8));
	bitarray_set(array, 12, 0);
	assert(42 == bitarray_firstset(array, sizeof(array) * 8));
	bitarray_set(array, 42, 0);
}
