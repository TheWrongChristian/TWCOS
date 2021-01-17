#include <stdarg.h>

#if INTERFACE
#include <stdnoreturn.h>

#endif

#include "panic.h"

void kernel_wait(char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	kernel_vprintk(fmt, ap);
	va_end(ap);
}

noreturn void kernel_vpanic(char * fmt, va_list ap)
{
	arch_panic(fmt, ap);
}

noreturn void kernel_panic(char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	kernel_vpanic(fmt, ap);
	va_end(ap);
}
