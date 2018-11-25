#include "timer.h"

#if INTERFACE

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

	void (*cb)(void * p);
	void * p;

	timer_event_t * next;
};

#endif

static timer_t * timers;

void timer_init(timer_ops_t * ops)
{
	INIT_ONCE();

	timers = calloc(1, sizeof(*timers));
	timers->ops = ops;
	thread_gc_root(timers);
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
		timers->queue->usec = timers->ops->timer_clear();
	}
}

static void timer_expire()
{
	SPIN_AUTOLOCK(timers->lock) {
		timers->running = 0;
		timer_event_t * timer = timers->queue;
		if (timer) {
			/* Remove from queue */
			timers->queue = timer->next;
			timer->next = 0;

			spin_unlock(timers->lock);
			/* Call the callback */
			timer->cb(timer->p);
			spin_lock(timers->lock);

			/* Start next timer */
			timer_set();
		} else {
			/* FIXME: Spurious timer */
		}
	}
}

timer_event_t * timer_add(timerspec_t usec, void (*cb)(void * p), void * p)
{
	timer_event_t * timer = malloc(sizeof(*timer));

	SPIN_AUTOLOCK(timers->lock) {
		timer_event_t * next = timers->queue;
		timer_event_t ** pprev = &timers->queue;

		timer->usec = usec;
		timer->cb = cb;
		timer->p = p;
		timer->next = 0;

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

	return timer;
}

void timer_delete(timer_event_t * timer)
{
	SPIN_AUTOLOCK(timers->lock) {
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

static void timer_sleep_cb(void * p)
{
	monitor_t * lock = p;

	MONITOR_AUTOLOCK(lock) {
		monitor_signal(lock);
	}
}

void timer_sleep(timerspec_t usec)
{
	timer_event_t * timer;
	monitor_t lock[1] = {0};

	MONITOR_AUTOLOCK(lock) {
		timer = timer_add(usec, timer_sleep_cb, lock);
		monitor_wait(lock);
	}
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
