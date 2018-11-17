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
	process_t * process;

	/* Run state */
	tstate state;
	tpriority priority;

	/* Thread information */
	char * name;

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


#endif

static tls_key tls_next = 1;
static map_t * allthreads;

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
	spin_lock(&queuelock);
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

char * thread_get_name(thread_t * thread)
{
	if (0 == thread->name) {
		thread->name = "Anonymous";
	}

	return thread->name;
}

void thread_set_name(thread_t * thread, char * name)
{
	thread->name = name;
}

thread_t * thread_fork()
{
	thread_t * this = arch_get_thread();
	thread_t * thread = slab_alloc(threads);

	thread->priority = this->priority;
	thread->process = this->process;
	char buf[32];
	snprintf( buf, sizeof(buf), "Child of %p", this);
	thread_set_name(thread, strndup(buf, sizeof(buf)));

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
	map_removepp(allthreads, this);

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

static map_t * roots;

static void thread_gc_walk(void * p, void * key, void * d)
{
	slab_gc_mark(key);
}

void thread_gc()
{
	thread_cleanlocks();
	slab_gc_begin();
	slab_gc_mark(arch_get_thread());
	for(int i=0; i<sizeof(queue)/sizeof(queue[0]); i++) {
		slab_gc_mark(queue[i]);
	}
	slab_gc_mark(roots);
	slab_gc_end();
}

void thread_gc_root(void * p)
{
	if (0 == roots) {
		roots = arraymap_new(0, 128);
	}
	map_putpp(roots, p, p);
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
	sync_init();
	arch_thread_init(slab_alloc(threads));
	allthreads = tree_new(0, TREE_SPLAY);
	thread_gc_root(allthreads);
	map_putpp(allthreads, arch_get_thread(), arch_get_thread() );
}

static void thread_test2();
static void thread_test1(rwlock_t * rw)
{
	void ** bt = thread_backtrace(15);
	kernel_printk("thread_test1\n");
	while(*bt) {
		kernel_printk("\t%p\n", *bt++);
	}

	rwlock_escalate(rw);
	rwlock_read(rw);
	rwlock_unlock(rw);
}

static void thread_test2(rwlock_t * rw)
{
	void ** bt = thread_backtrace(15);
	kernel_printk("thread_test2\n");
	while(*bt) {
		kernel_printk("\t%p\n", *bt++);
	}
	rwlock_unlock(rw);
	rwlock_write(rw);
	rwlock_unlock(rw);
}

void thread_test()
{
	thread_t * thread1;

	thread1 = thread_fork();
	if (thread1) {
		thread_join(thread1);
		thread1 = 0;
	} else {
		static rwlock_t rw[1] = {{0}};
		thread_t * thread2 = thread_fork();
		rwlock_read(rw);
		if (thread2) {
			thread_test1(rw);
			thread_join(thread2);
			thread2 = 0;
			thread_exit(0);
		} else {
			thread_test2(rw);
			thread_exit(0);
		}
	}
}
