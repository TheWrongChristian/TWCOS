#include "exception.h"

#if INTERFACE
#include <setjmp.h>
#include <stdarg.h>

struct exception_def {
	const char * name;
	struct exception_def * parent;
};

struct exception_cause {
	struct exception_def * type;
	
	/* Exception location */
	const char * file;
	const int line;

	/* Exception message */
	char message[256];
};

struct exception_frame {
	jmp_buf env;

	/* Try block location */
	const char * file;
	const int line;

	/* Exception cause if thrown */
	struct exception_cause * cause;
};

/*
 * Top level exception from which all others are derived
 */
static struct exception_def exception_def_Throwable = { "Throwable", 0 };

#define EXCEPTION_DEF(type,parent) static struct exception_def exception_def_ ## type = { #type, &ctk_exception_def_ ## parent }

#endif
