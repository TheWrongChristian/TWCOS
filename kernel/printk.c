#include "printk.h"

#if INTERFACE

#include <stdarg.h>

typedef void (*kernel_logger)(void * p, const char * msg);

struct kernel_logger_listener_t {
	int level;
	kernel_logger cb;
};

#endif

static char msg_ring[32][128];
static int msg_next=0;
static int enabled=0;
static char * msgs[countof(msg_ring)];

stream_t * console = 0;

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

static kernel_logger_listener_t listener[1];

void kernel_vlog(int level, const char * fmt, va_list ap)
{
	if (listener->cb && level<listener->level) {
	}
}

void kernel_log(int level, const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlog(level, fmt, ap);
	va_end(ap);
}
