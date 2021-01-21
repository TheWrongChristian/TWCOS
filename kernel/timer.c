#include <sys/types.h>
#include "timer.h"

#if INTERFACE

#include <stdint.h>

typedef int64_t timerspec_t;

#if 0
struct timer_ops_t {
	void (*timer_set)(void (*expire)(), timerspec_t usec);
	timerspec_t (*timer_clear)();
};
#endif

struct timer_t {
	timer_event_t * queue;
	timer_event_t * expired;
	thread_t * thread;
	int running;
};

struct timer_event_t {
	timerspec_t usec;
	timerspec_t reset;

	void (*cb)(void * p);
	void * p;

	timer_event_t * next;
	timer_event_t * prev;
};

#endif

static interrupt_monitor_t * timers_lock = 0;
static GCROOT timer_t timers[1] = {0};
static timerspec_t uptime = 0;
static GCROOT timer_event_t * uptime_timer = 0;

static void timer_event_mark(void * p)
{
	void * next;
	INTERRUPT_MONITOR_AUTOLOCK(timers_lock) {
		timer_event_t * timer = p;
		p = timer->p;
		next = timer->next;
	}
	slab_gc_mark(p);
	slab_gc_mark(next);
}

#if 0
static void timer_event_finalize(void * p)
{
	timer_event_t * timer = p;
	timer->next = 0;
}
#else
#define timer_event_finalize (0)
#endif

static slab_type_t timer_events[1] = { SLAB_TYPE(sizeof(timer_event_t), timer_event_mark, timer_event_finalize)};

static void timer_uptime_cb(void * ignored)
{
	/* Restart */
	timer_start(uptime_timer);
}

static void timer_expire();
static void timer_set()
{
	if (timers->queue && !timers->running) {
		timers->running = 1;
		arch_timer_set(timer_expire, timers->queue->usec);
	}
}

static void timer_clear()
{
	if (timers->queue && timers->running) {
		timers->running = 0;
		timerspec_t remaining = arch_timer_clear();
		uptime += (timers->queue->usec - remaining);
		timers->queue->usec = remaining;
	}
}

/*
 * Post execution of timer_expire, timers->queue will be
 * advanced past all the expired timers.
 *
 * At this point, the timer thread will kick in and process
 * the expired timers until we catch back up to the
 * outstanding queue.
 */
static void timer_expire()
{
	/* We're called in interrupt context */
	timers->running = 0;
	if (timers->queue) {
		/* Find all of the timers to expire */
		timer_event_t * next = timers->queue;
		uptime += next->usec;
		do {
			LIST_DELETE(timers->queue, next);
			LIST_APPEND(timers->expired, next);
			next = timers->queue;
		} while(next && 0 == next->usec);

		/* Start next timer */
		if (timers->queue) {
			arch_timer_set(timer_expire, timers->queue->usec);
		}

		/* Poke other threads */
		interrupt_monitor_broadcast(timers_lock);
	}
}

static void timer_expire_thread()
{
	while(1) {
		timer_event_t * expired = 0;
		INTERRUPT_MONITOR_AUTOLOCK(timers_lock) {
			while(0 == timers->expired) {
				interrupt_monitor_wait(timers_lock);
			}
			expired = timers->expired;
			timers->expired = 0;
		}
		/* Finally, process expired timers */
		while(expired) {
			timer_event_t * timer = expired;
			LIST_DELETE(expired, timer);
			timer->cb(timer->p);
		}
	}
}

timer_event_t * timer_add(timerspec_t usec, void (*cb)(void * p), void * p)
{
	timer_event_t * timer = slab_alloc(timer_events);
	timer->reset = usec;
	timer->cb = cb;
	timer->p = p;
	timer->next = 0;
	timer->prev = 0;

	timer_start(timer);

	return timer;
}

void timer_start(timer_event_t * timer)
{
	timer->usec = timer->reset;

	INTERRUPT_MONITOR_AUTOLOCK(timers_lock) {
		timer_event_t * next = timers->queue;
		if (next) {
			/* Cancel the current outstanding timer */
			timer_clear();
		}

		while(next) {
			assert(next != timer);
			if (timer->usec < next->usec) {
				next->usec -= timer->usec;
				break;
			} else {
				timer->usec -= next->usec;
			}
			LIST_NEXT(timers->queue, next);
		};

		/* Put into the queue */
		LIST_INSERT_BEFORE(timers->queue, next, timer);

		/* Set the timer */
		timer_set();
	}
}

void timer_delete(timer_event_t * timer)
{
	INTERRUPT_MONITOR_AUTOLOCK(timers_lock) {
		timer_clear();
		LIST_DELETE(timers->queue, timer);
		timer->cb = 0;
		timer->p = 0;

		/* Set the timer */
		timer_set();
	}
}

timerspec_t timer_uptime()
{
#if 1
	timerspec_t t = 0;
	INTERRUPT_MONITOR_AUTOLOCK(timers_lock) {
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
	interrupt_monitor_t lock[1];
};

static void timer_sleep_cb(void * p)
{
	struct sleepvar * sleep = p;

	INTERRUPT_MONITOR_AUTOLOCK(sleep->lock) {
		sleep->done = 1;
		interrupt_monitor_broadcast(sleep->lock);
	}
}

void timer_sleep(timerspec_t usec)
{
	struct sleepvar sleep[1] = {{0}};

	INTERRUPT_MONITOR_AUTOLOCK(sleep->lock) {
		timer_add(usec, timer_sleep_cb, sleep);
		while(!sleep->done) {
			interrupt_monitor_wait(sleep->lock);
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

void timer_init()
{
	INIT_ONCE();

#if 0
	timers->ops = arch_timer_ops();


	timers_lock = interrupt_monitor_irq(0);
	intr_add(0, timer_expire(), 0);
#endif
	timers_lock = arch_timer_init();

	timers->thread = thread_fork();
	if (0 == timers->thread) {
		timer_expire_thread();
	}

	/* Uptime tracking timer - update uptime at least every 1 second */
	uptime_timer = timer_add(1000000, timer_uptime_cb, 0);
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
