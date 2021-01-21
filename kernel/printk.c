#include "printk.h"

#if INTERFACE

#include <stdarg.h>

enum logger_level {
	logger_error = 0,
	logger_warning,
	logger_warn = logger_warning,
	logger_information,
	logger_info = logger_information,
	logger_debug
};

#ifdef DEBUG
#define TRACE() kernel_debug("... %s:%d\n", __FILE__, __LINE__)
#else
#define TRACE() do {} while(0)
#endif

#endif

static char msg_ring[32][128];
static int msg_next=0;
static int enabled=0;
static char * msgs[countof(msg_ring)];
static interrupt_monitor_t loggerlock[1];
static GCROOT thread_t * logger=0;

stream_t * logging_stream = 0;

static void kernel_logger()
{
	INTERRUPT_MONITOR_AUTOLOCK(loggerlock) {
		int lastlog = 0;
		while(1) {
			if (msg_next>=lastlog+countof(msg_ring)) {
				lastlog = msg_next-countof(msg_ring)+1;
			}
			for(; lastlog<msg_next; lastlog++) {
				int msg = lastlog % countof(msg_ring);
				stream_putstr(logging_stream, msg_ring[msg]);
			}
			interrupt_monitor_wait(loggerlock);
		}
	}
}

void kernel_startlogging(int enable)
{
	enabled = enable;
	if (0 == logging_stream) {
		logging_stream = console_stream();
		thread_t * thread = thread_fork();
		if (thread) {
			logger = thread;
		} else {
			kernel_logger();
		}
	}
}

void kernel_printk(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);

        kernel_vprintk(fmt, ap);

        va_end(ap);
}

void kernel_vprintk(const char * fmt, va_list ap)
{
	if (!enabled) {
		return;
	}

	kernel_info(fmt, ap);
}

void kernel_vlogger(logger_level level, const char * fmt, va_list ap)
{
	static char levels [] = { 'E', 'W', 'I', 'D' };
	check_int_bounds(level, 0, countof(levels)-1, "Log level out of range");
	static thread_t * owner = 0;

	if (owner && owner == arch_get_thread())
	{
		// Recursing
		return;
	}
	INTERRUPT_MONITOR_AUTOLOCK(loggerlock) {
		int msg = msg_next++ % countof(msg_ring);
		owner = arch_get_thread();
		msg_ring[msg][0] = levels[level];
		msg_ring[msg][1] = ':';
		vsnprintf(msg_ring[msg]+2, countof(msg_ring[msg])-2, fmt, ap);
		owner = 0;
		interrupt_monitor_signal(loggerlock);
	}
}

void kernel_error(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_error, fmt, ap);
	va_end(ap);
}

void kernel_warn(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_warning, fmt, ap);
	va_end(ap);
}

void kernel_warning(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_warning, fmt, ap);
	va_end(ap);
}

void kernel_info(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_debug, fmt, ap);
	va_end(ap);
}

void kernel_information(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_debug, fmt, ap);
	va_end(ap);
}

void kernel_debug(const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(logger_debug, fmt, ap);
	va_end(ap);
}

#if 0
void kernel_logger(logger_level level, const char * fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	kernel_vlogger(level, fmt, ap);
	va_end(ap);
}
#endif
