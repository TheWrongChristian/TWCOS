#include "check.h"

exception_def CheckException = { "CheckException", &Exception };
exception_def NullPointerException = { "NullPointerException", &CheckException };
exception_def IntBoundsException = { "IntBoundsException", &CheckException };
exception_def PtrBoundsException = { "PtrBoundsException", &CheckException };
exception_def IntValueException = { "IntValueException", &CheckException };
exception_def InvalidPointerException = { "InvalidPointerException", &CheckException };

void check_not_null(void * p, const char * error)
{
	if (0 == p) {
		KTHROWF(NullPointerException, "%s: %p", error, p);
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

static void check_user_ptr(void * p, size_t len, int write, const char * error)
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
