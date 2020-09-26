#include "future.h"

#if INTERFACE

struct future_t {
	interrupt_monitor_t lock[1];
	int pending;
	exception_cause * cause;
	int status;
	void (*cleanup)(void * p);
	void * p;
};


#endif

exception_def FutureException = { "FutureException", &Exception };
exception_def CancelledFutureException = { "CancelledFutureException", &FutureException };

int future_get(future_t * future)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		while(future->pending) {
			interrupt_monitor_wait(future->lock);
		}
	}

	if (future->cleanup) {
		future->cleanup(future->p);
	}

	if (future->cause) {
		KTHROWC(future->cause);
	}

	return future->status;
}

void future_set(future_t * future, int status)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		future->status = status;
		future->pending = 0;
		interrupt_monitor_broadcast(future->lock);
	}
}

future_t * future_create(void (*cleanup)(void * p), void * p)
{
	future_t * future = calloc(1, sizeof(*future));
	future->pending = 1;
	future->cleanup = cleanup;
	future->p = p;

	return future;
}

void future_pending(future_t * future)
{
	/* This should be done to initialise, so no lock needed */
	memset(future, 0, sizeof(*future));
	future->pending = 1;
}

void future_cancel(future_t * future, exception_cause * cause)
{
	INTERRUPT_MONITOR_AUTOLOCK(future->lock) {
		future->pending = 0;
		future->cause = cause;
	}
}
