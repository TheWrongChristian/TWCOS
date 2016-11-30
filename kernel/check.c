#include "check.h"

void check_not_null(void * p, const char * error)
{
	if (0 == p) {
		kernel_panic("%s: %p\n", error, p);
	}
}

void check_int_bounds(int i, int low, int high, const char * error)
{
	if (i<low || i>high) {
		kernel_panic("%s: %d\n", error, i);
	}
}
