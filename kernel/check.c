#include "check.h"

/**
 * Base check exception
 */
exception_def CheckException = { "CheckException", &Exception };

/**
 * Integer bounds check exception
 */
exception_def IntBoundsException = { "IntBoundsException", &CheckException };

/**
 * Pointer bounds check exception
 */
exception_def PtrBoundsException = { "PtrBoundsException", &CheckException };

/**
 * Integer value check exception
 */
exception_def IntValueException = { "IntValueException", &CheckException };

/**
 * Invalid pointer exception
 */
exception_def InvalidPointerException = { "InvalidPointerException", &CheckException };

/**
 * NULL pointer exception
 */
exception_def NullPointerException = { "NullPointerException", &InvalidPointerException };

/**
 * Check a pointer is not NULL.
 * \arg p Pointer to check.
 * \arg error Error message associated with the NULL pointer.
 * \throws NullPointerException if p is NULL
 */
void check_not_null(void * p, const char * error)
{
	if (0 == p) {
		KTHROWF(NullPointerException, "%s: %p", error, p);
	}
}

/**
 * Check an integer value is between two values (inclusive).
 * \arg i Integer to check.
 * \arg low Integer range low.
 * \arg high Integer range high.
 * \arg error Error message associated with the integer range.
 * \throws IntBoundsException if p is NULL
 */
void check_int_bounds(int i, int low, int high, const char * error)
{
	if (i<low || i>high) {
		KTHROWF(IntBoundsException, "%s: %d", error, i);
	}
}

/**
 * Check a pointer value is between two values (inclusive).
 * \arg i Pointer to check.
 * \arg low Pointer range low.
 * \arg high Pointer range high.
 * \arg error Error message associated with the integer range.
 * \throws PtrBoundsException if p is NULL
 */
void check_ptr_bounds(void * p, void * low, void * high, const char * error)
{
	if ((char*)p<(char*)low || (char*)p>=(char*)high) {
		KTHROWF(PtrBoundsException, "%s: %p", error, p);
	}
}

/**
 * Check an integer value matches an integer.
 * \arg i Integer to check.
 * \arg high Integer value to match against.
 * \arg error Error message associated with the integer match.
 * \throws IntValueException if the integer value does not match.
 */
void check_int_is(int i, int value, const char * error)
{
	if (i != value) {
		KTHROWF(IntValueException, "%s: %d", error, i);
	}
}

/**
 * Check a pointer value is between two values (inclusive).
 * \arg p Pointer to check.
 * \arg len Size of user data being checked.
 * \arg write 1 (true) if the access required is a write.
 * \arg error Error message associated with the integer range.
 * \throws InvalidPointerException if pointer does not allow the access to the user data.
 */
void check_user_ptr(void * p, size_t len, int write, const char * error)
{
	/*
	 * All pages between vpstart and vpend must be amenable to
	 * use as desired.
	 */
	char * vpstart = ARCH_PAGE_ALIGN(p);
	char * vpend = p;

	vpend += len + ARCH_PAGE_SIZE;;
	vpend = ARCH_PAGE_ALIGN(vpend);

	map_t * as = (arch_get_thread()->process) ? arch_get_thread()->process->as : 0;
	segment_t * seg = (as) ? map_getpp_cond(arch_get_thread()->process->as, vpstart, MAP_LE) : 0;
	while(seg) {
		if (write && 0 == (seg->perms&SEGMENT_W)) {
			KTHROWF(InvalidPointerException, "%s: %p", error, p);
		}

		/* Check if we need to move to the next segment */
		char * segend = seg->base;
		segend += seg->size;

		if (vpend >= segend) {
			/* Get the next contiguous segment, if any */
			seg = map_getpp(as, segend);
		} else {
			/* Within the bounds of this segment */
			return;
		}
	}

	/*
	 * If we et here, we've run out of contiguous segments
	 */
	KTHROWF(InvalidPointerException, "%s: %p", error, p);
}
