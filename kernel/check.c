#include "check.h"

exception_def CheckException = { "CheckException", &Exception };
exception_def NullCheckException = { "NullCheckException", &CheckException };
exception_def IntBoundsException = { "IntBoundsException", &CheckException };
exception_def PtrBoundsException = { "PtrBoundsException", &CheckException };
exception_def IntValueException = { "IntValueException", &CheckException };

void check_not_null(void * p, const char * error)
{
	if (0 == p) {
		KTHROWF(NullCheckException, "%s: %p", error, p);
	}
}

void check_int_bounds(int i, int low, int high, const char * error)
{
	if (i<low || i>high) {
		KTHROWF(IntBoundsException, "%s: %d", error, i);
	}
}

void check_ptr_bounds(void * p, void * low, void * high, const char * error)
{
	if ((char*)p<(char*)low || (char*)p>=(char*)high) {
		KTHROWF(PtrBoundsException, "%s: %d", error, p);
	}
}

void check_int_is(int i, int value, const char * error)
{
	if (i != value) {
		KTHROWF(IntValueException, "%s: %d", error, i);
	}
}
