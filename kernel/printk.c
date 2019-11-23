#include <stdarg.h>

#include "printk.h"


stream_t * console = 0;

#define countof(arr) (sizeof(arr)/sizeof(arr[0]))

static char msg_ring[32][128];
static int msg_next=0;
static int enabled=0;
static char * msgs[countof(msg_ring)];

void kernel_startlogging(int enable)
{
	enabled = enable;
	if (0 == console) {
		console = console_stream();
	}
}

int kernel_printk(const char * fmt, ...)
{
        int len = 0;
        va_list ap;
        va_start(ap, fmt);

        len = kernel_vprintk(fmt, ap);

        va_end(ap);

        return len;
}

int kernel_vprintk(const char * fmt, va_list ap)
{
        int len = 0;

	if (!enabled) {
		return 0;
	}

	len = vsnprintf(msg_ring[msg_next], countof(msg_ring[msg_next]), fmt, ap);
	for(int i=0; i<countof(msgs); i++) {
		msgs[i]=msg_ring[(1+i+msg_next)%countof(msg_ring)];
	}
	if (++msg_next == countof(msg_ring)) {
		msg_next=0;
	}

        return len;
}
