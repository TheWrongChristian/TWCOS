#include <stdarg.h>

#include "panic.h"

void kernel_panic(char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	arch_panic(fmt, ap);
	va_end(ap);
}
