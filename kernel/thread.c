#include "thread.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>

typedef int tls_key;

#define TLS_MAX 32
struct thread_t {
	/* Runtime data */
	void * tls[TLS_MAX];
	arch_context_t context;
	map_t * as;

	/* Run state */
	tstate state;
	tpriority priority;

	/* Return value */
	void * retval;

	/* Queue */
	thread_t *prev;
	thread_t *next;
};

enum tstate { THREAD_NEW, THREAD_RUNNABLE, THREAD_RUNNING, THREAD_SLEEPING, THREAD_TERMINATED };
enum tpriority { THREAD_INTERRUPT = 0, THREAD_NORMAL, THREAD_IDLE, THREAD_PRIORITIES };

#define INIT_ONCE() \
	do { \
		static int inited = 0; \
		if (inited) { \
			return; \
		} \
		inited = 1; \
	} while(0)

#define SPIN_AUTOLOCK(lock) int s##__LINE__ = 0; while((s##__LINE__=spin_autolock(lock, s##__LINE__)))

#endif

static tls_key tls_next = 1;

int tls_get_key()
{
	return arch_atomic_postinc(&tls_next);
}


void tls_set(int key, void * p)
{
	thread_t * thread = arch_get_thread();

	check_not_null(thread, "Unable to get thread");
	check_int_bounds(key, 1, TLS_MAX-1, "TLS key out of bounds");

	thread->tls[key] = p;
}

void * tls_get(int key)
{
	thread_t * thread = arch_get_thread();

	check_not_null(thread, "Unable to get thread");
	check_int_bounds(key, 1, TLS_MAX-1, "TLS key out of bounds");

	return thread->tls[key];
}

static void thread_mark(void * p);
static void thread_finalize(void * p);
static slab_type_t threads[1] = {SLAB_TYPE(sizeof(thread_t), thread_mark, thread_finalize)};

thread_t * thread_prequeue(thread_t * queue, thread_t * thread, tstate state)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}
	thread->state = state;
	LIST_PREPEND(queue, thread);
	return queue;
}

thread_t * thread_queue(thread_t * queue, thread_t * thread, tstate state)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}
	thread->state = state;
	LIST_APPEND(queue, thread);
	return queue;
}

/* Simple RR scheduler */
static thread_t * queue[THREAD_PRIORITIES];
static int queuelock;

static void scheduler_lock()
{
	while(!spin_trylock(&queuelock)) {
	}
	return;
}

static void scheduler_unlock()
{
	spin_unlock(&queuelock);
}

void thread_preempt()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_prequeue(queue[priority], this, THREAD_RUNNABLE);
	scheduler_unlock();
	thread_schedule();
}

void thread_yield()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], this, THREAD_RUNNABLE);
	scheduler_unlock();
	thread_schedule();
}

void thread_resume(thread_t * thread)
{
	tpriority priority = thread->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], thread, THREAD_RUNNABLE);
	scheduler_unlock();
}

void thread_schedule()
{
	int i;
	scheduler_lock();
	for(i=0; i<THREAD_PRIORITIES; i++) {
		if (0 == queue[i]) {
			continue;
		} else {
			thread_t * next = queue[i];
			LIST_DELETE(queue[i], next);
			scheduler_unlock();
			arch_thread_switch(next);
			return;
		}
	}
	kernel_panic("Empty run queue!\n");
}

thread_t * thread_fork()
{
	thread_t * this = arch_get_thread();
	thread_t * thread = slab_alloc(threads);

	thread->priority = this->priority;
	thread->as = this->as;

	if (0 == arch_thread_fork(thread)) {
		return 0;
	}

	thread_resume(thread);

	return thread;
}

void thread_exit(void * retval)
{
	thread_t * this = arch_get_thread();

	this->retval = retval;
	this->state = THREAD_TERMINATED;

	/* Signify to any threads waiting on this thread */
	thread_lock(this);
	thread_broadcast(this);
	thread_unlock(this);

	/* Schedule the next thread */
	thread_schedule();
}

void * thread_join(thread_t * thread)
{
	void * retval = 0;
	thread_lock(thread);
	while(thread->state != THREAD_TERMINATED) {
		thread_wait(thread);
	}
	retval = thread->retval;
	thread_unlock(thread);

	/* FIXME: Clean up thread resources */

	return retval;
}

void thread_set_priority(thread_t * thread, tpriority priority)
{
	check_int_bounds(priority, THREAD_INTERRUPT, THREAD_IDLE, "Thread priority out of bounds");
	if (0 == thread) {
		thread = arch_get_thread();
	}
	thread->priority = priority;
}

void thread_gc()
{
	thread_cleanlocks();
	slab_gc_begin();
	slab_gc_mark(arch_get_thread());
	slab_gc_mark(kas);
	for(int i=0; i<sizeof(queue)/sizeof(queue[0]); i++) {
		slab_gc_mark(queue[i]);
	}
	slab_gc_mark(locktable);
	slab_gc_end();
}

static void thread_mark(void * p)
{
	thread_t * thread = (thread_t *)p;

	for(int i=0; i<sizeof(thread->tls)/sizeof(thread->tls[0]); i++) {
		slab_gc_mark(thread->tls[i]);
	}
	slab_gc_mark(thread->next);
	slab_gc_mark(thread->retval);

	arch_thread_mark(thread);
}

static void thread_finalize(void * p)
{
	thread_t * thread = (thread_t *)p;
	arch_thread_finalize(thread);
}

void ** thread_backtrace(int levels)
{
	return arch_thread_backtrace(levels);
}

void thread_init()
{
	INIT_ONCE();

	/* Craft a new bootstrap thread to replace the static defined thread */
	arch_thread_init(slab_alloc(threads));
}

static void thread_test2();
static void thread_test1()
{
	void ** bt = thread_backtrace(15);
	kernel_printk("thread_test1\n");
	while(*bt) {
		kernel_printk("\t%p\n", *bt++);
	}
}

static void thread_test2()
{
	void ** bt = thread_backtrace(15);
	kernel_printk("thread_test2\n");
	while(*bt) {
		kernel_printk("\t%p\n", *bt++);
	}
}

void thread_test()
{
	thread_t * thread1;

	thread1 = thread_fork();
	if (thread1) {
		thread_join(thread1);
		thread1 = 0;
	} else {
		thread_t * thread2 = thread_fork();
		if (thread2) {
			thread_test1();
			thread_join(thread2);
			thread2 = 0;
			thread_exit(0);
		} else {
			thread_test2();
			thread_exit(0);
		}
	}
}
