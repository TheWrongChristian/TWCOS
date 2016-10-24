#include <stdarg.h>

#include "printk.h"


struct stream * console = 0;

int kernel_printk(const char * fmt, ...)
{
        int len = 0;
        va_list ap;
        va_start(ap, fmt);

	if (0 == console) {
		console = console_stream();
	}

        len = stream_vprintf(console, fmt, ap);

        va_end(ap);

        return len;
}

int kernel_vprintk(const char * fmt, va_list ap)
{
        int len = 0;

	if (0 == console) {
		console = console_stream();
	}

        len = stream_vprintf(console, fmt, ap);

        va_end(ap);

        return len;
}
