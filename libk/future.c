#include "future.h"

#if INTERFACE

struct future_t {
	interrupt_monitor_t lock[1];
	volatile int pending;
	exception_cause * cause;
	intptr_t status;
	intptr_t success;
	void (*cleanup)(void * p);
	void * p;
	future_t * prev;
};


#endif

exception_def FutureException = { "FutureException", &Exception };
exception_def CancelledFutureException = { "CancelledFutureException", &FutureException };

intptr_t future_get_timeout(future_t * future, timerspec_t timeout)
{
	if (future->prev) {
		/* Ignore status from previous futures */
		(void)future_get_timeout(future->prev, timeout);
	}

	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		while(future->pending) {
			interrupt_monitor_wait_timeout(future->lock, timeout);
		}
	}

	if (future->cleanup) {
		future->cleanup(future->p);
		future->cleanup = 0;
	}

	if (future->cause) {
		/* Suppress further exceptions from this future */
		exception_cause * cause = future->cause;
		future->cause = 0;

		/* Throw the original exception */
		KTHROWC(cause);
	}

	return future->status;
}

intptr_t future_get(future_t * future)
{
	return future_get_timeout(future, 0);
}

void future_set(future_t * future, intptr_t status)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		if (future->pending) {
			future->status = status;
			future->pending = 0;
			interrupt_monitor_broadcast(future->lock);
		}
	}
}

void future_init(future_t * future, void (*cleanup)(void * p), void * p)
{
	memset(future, 0, sizeof(*future));
	future->pending = 1;
	future->cleanup = cleanup;
	future->p = p;
	future->prev = 0;
}

future_t * future_create(void (*cleanup)(void * p), void * p)
{
	future_t * future = malloc(sizeof(*future));
	future_init(future, cleanup, p);
	return future;
}

void future_cancel(future_t * future, exception_cause * cause)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		if (future->pending) {
			future->pending = 0;
			future->cause = cause;
			interrupt_monitor_broadcast(future->lock);
		}
	}
}

void future_chain(future_t * future, future_t * prev)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		future->prev = prev;
	}
}

future_t * future_static_success()
{
	static future_t future[]={{{0}}};

	return future;
}
