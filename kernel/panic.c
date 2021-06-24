#include <stdarg.h>

#if INTERFACE
#include <stdnoreturn.h>

#endif

#include "panic.h"

void kernel_wait(const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	kernel_vprintk(fmt, ap);
	va_end(ap);
}

noreturn void kernel_vpanic(const char * fmt, va_list ap)
{
	void * backtrace[32];
	kernel_vprintk(fmt, ap);
	kernel_backtrace(logger_error);
	arch_panic(fmt, ap);
}

noreturn void kernel_panic(const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	kernel_vpanic(fmt, ap);
	va_end(ap);
}
