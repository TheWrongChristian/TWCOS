#include "timer.h"

#if INTERFACE

#include <stdint.h>
#include <unistd.h>

typedef int64_t timerspec_t;

struct timer_ops_t {
	void (*timer_set)(void (*expire)(), timerspec_t usec);
	timerspec_t (*timer_clear)();
};

struct timer_t {
	timer_event_t * queue;
	timer_ops_t * ops;
	int running;
	int lock[1];
};

struct timer_event_t {
	timerspec_t usec;
	timerspec_t reset;

	void (*cb)(void * p);
	void * p;

	timer_event_t * next;
};

#endif

static int timers_lock[1]={0};
static GCROOT timer_t timers_static;
static timer_t * timers = &timers_static;
static timerspec_t uptime = 0;
static timer_event_t * uptime_timer = 0;

static void timer_uptime_cb(void * ignored)
{
	/* Restart */
	timer_start(uptime_timer);
}

void timer_init(timer_ops_t * ops)
{
	INIT_ONCE();

	timers->ops = ops;

	/* Uptime tracking timer - update uptime at least every 1 second */
	uptime_timer = timer_add(1000000, timer_uptime_cb, 0);
}

static void timer_expire();
static void timer_set()
{
	if (timers->queue && !timers->running) {
		timers->running = 1;
		timers->ops->timer_set(timer_expire, timers->queue->usec);
	}
}

static void timer_clear()
{
	if (timers->queue && timers->running) {
		timers->running = 0;
		timerspec_t remaining = timers->ops->timer_clear();
		uptime += (timers->queue->usec - remaining);
		timers->queue->usec = remaining;
	}
}

static void timer_expire()
{
	SPIN_AUTOLOCK(timers_lock) {
		timers->running = 0;
		timer_event_t * timer = timers->queue;
		if (timer) {
			/* Remove from queue */
			timers->queue = timer->next;
			timer->next = 0;
			uptime += timer->usec;

			spin_unlock(timers_lock);
			/* Call the callback */
			timer->cb(timer->p);
			spin_lock(timers_lock);

			/* Start next timer */
			timer_set();
		} else {
			/* FIXME: Spurious timer */
		}
	}
}

timer_event_t * timer_add(timerspec_t usec, void (*cb)(void * p), void * p)
{
	timer_event_t * timer = calloc(1, sizeof(*timer));
	timer->usec = usec;
	timer->reset = usec;
	timer->cb = cb;
	timer->p = p;
	timer->next = 0;

	timer_start(timer);

	return timer;
}

void timer_start(timer_event_t * timer)
{
	timer->usec = timer->reset;

	SPIN_AUTOLOCK(timers_lock) {
		timer_event_t * next = timers->queue;
		timer_event_t ** pprev = &timers->queue;

		if (next) {
			/* Cancel the current outstanding timer */
			timer_clear();
		}

		while(next) {
			if (timer->usec < next->usec) {
				next->usec -= timer->usec;
				break;
			} else {
				timer->usec -= next->usec;
				pprev = &next->next;
				next = next->next;
			}
		};

		/* Put into the queue */
		timer->next = *pprev;
		*pprev = timer;

		/* Set the timer */
		timer_set();
	}
}

void timer_delete(timer_event_t * timer)
{
	SPIN_AUTOLOCK(timers_lock) {
		timer_event_t * next = timers->queue;
		timer_event_t ** pprev = &timers->queue;

		timer_clear();
		while(next) {
			if (next == timer) {
				if (timer->next) {
					timer->next->usec += timer->usec;
				}

				*pprev = timer->next;
				timer->next = 0;
				timer->cb = 0;
				timer->p = 0;
				break;
			} else {
				next = next->next;
			}
		}

		/* Set the timer */
		timer_set();
	}
}

timerspec_t timer_uptime()
{
#if 1
	timerspec_t t = 0;
	SPIN_AUTOLOCK(timers_lock) {
		timer_clear();
		t = uptime;
		timer_set();
	}
#else
	timerspec_t t = uptime;
#endif
	return t;
}

struct sleepvar {
	volatile int done;
	thread_t * waiting;
	int lock[1];
};

static void timer_sleep_cb(void * p)
{
	struct sleepvar * sleep = p;

	assert(sleep->waiting);

	SPIN_AUTOLOCK(sleep->lock) {
		sleep->done = 1;
		thread_t * resume = sleep->waiting;
		LIST_DELETE(sleep->waiting, resume);
		thread_resume(resume);
	}
}

void timer_sleep(timerspec_t usec)
{
	struct sleepvar sleep[1] = {{0}};

	SPIN_AUTOLOCK(sleep->lock) {
		timer_add(usec, timer_sleep_cb, sleep);
		sleep->waiting = thread_queue(sleep->waiting, NULL, THREAD_SLEEPING);
		while(!sleep->done) {
			spin_unlock(sleep->lock);
			thread_schedule();
			spin_lock(sleep->lock);
		}
	}
}

int timer_nanosleep(struct timespec * req, struct timespec * rem)
{
	/* TODO: Do this properly, filling in rem if required */
	timer_sleep(req->tv_sec*1000000 + req->tv_nsec/1000);

	return 0;
}

static timer_event_t * test_timer;
static void timer_test_cb(void * p);
static void timer_start_timer()
{
	test_timer = timer_add(500000, timer_test_cb, 0);
}

static void timer_test_cb(void * p)
{
	kernel_printk(".");
	timer_start_timer();
}

void timer_test()
{
	if (thread_fork()) {
		return;
	}
	kernel_printk("Sleeping for 1 second");
	timer_start_timer();
	timer_sleep(1000000);
	kernel_printk(" done\n");
	timer_delete(test_timer);
	kernel_printk("Sleeping for 2 second");
	timer_start_timer();
	timer_sleep(2000000);
	kernel_printk(" done\n");
	timer_delete(test_timer);
	thread_exit(0);
}
