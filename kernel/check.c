#include "check.h"

struct exception_def exception_def_CheckException = { "CheckException", &exception_def_Exception };
struct exception_def exception_def_NullCheckException = { "NullCheckException", &exception_def_CheckException };
struct exception_def exception_def_IntBoundsException = { "IntBoundsException", &exception_def_CheckException };

void check_not_null(void * p, const char * error)
{
	if (0 == p) {
		KTHROWF(NullCheckException, "%s: %p\n", error, p);
	}
}

void check_int_bounds(int i, int low, int high, const char * error)
{
	if (i<low || i>high) {
		KTHROWF(IntBoundsException, "%s: %d\n", error, i);
	}
}
