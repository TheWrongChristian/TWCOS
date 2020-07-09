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

	/* Thread CPU usage */
	int acct;
	struct {
		timerspec_t tstart;
		timerspec_t tlen;
	} accts[64];

	/* Run state */
	tstate state;
	tpriority priority;

	/* Thread information */
	char * name;

	/* Return value */
	monitor_t lock[1];
	void * retval;

	/* Queue */
	thread_t *prev;
	thread_t *next;
};

enum tstate { THREAD_NEW, THREAD_RUNNABLE, THREAD_RUNNING, THREAD_SLEEPING, THREAD_TERMINATED };
enum tpriority { THREAD_INTERRUPT = 0, THREAD_NORMAL, THREAD_IDLE, THREAD_PRIORITIES };

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
static GCROOT thread_t * queue[THREAD_PRIORITIES];
static int queuelock;

static void scheduler_lock()
{
	spin_lock(&queuelock);
}

static void scheduler_unlock()
{
	spin_unlock(&queuelock);
}

int thread_preempt()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_prequeue(queue[priority], this, THREAD_RUNNABLE);
	scheduler_unlock();
	return thread_schedule();
}

int thread_yield()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], this, THREAD_RUNNABLE);
	scheduler_unlock();
	return thread_schedule();
}

void thread_resume(thread_t * thread)
{
	tpriority priority = thread->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], thread, THREAD_RUNNABLE);
	scheduler_unlock();
}

int thread_schedule()
{
	while(1) {
		int i;
		scheduler_lock();
		for(i=0; i<THREAD_PRIORITIES; i++) {
			if (queue[i]) {
				thread_t * current = arch_get_thread();
				thread_t * next = queue[i];
				LIST_DELETE(queue[i], next);
				scheduler_unlock();
				if (arch_get_thread() != next) {
					/* Thread is changing, do accounting and switch to next */
					current->accts[current->acct].tlen = timer_uptime() - current->accts[current->acct].tstart;
					current->acct++;
					if (sizeof(current->accts)/sizeof(current->accts[0]) == current->acct) {
						current->acct = 0;
					}
					arch_thread_switch(next);
					current->accts[current->acct].tstart = timer_uptime();
					return 1;
				} else {
					/* Restore thread state to running */
					current->state = THREAD_RUNNING;
					return 0;
				}
			}
		}
		// kernel_printk("Empty run queue!\n");
		scheduler_unlock();
		arch_idle();
	}
}

char * thread_get_name(thread_t * thread)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}

	if (0 == thread->name) {
		thread->name = "Anonymous";
	}

	return thread->name;
}

void thread_set_name(thread_t * thread, char * name)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}

	thread->name = name;
}

static void thread_track(thread_t * thread, int add)
{
	static int lock = 0;
	static GCROOT map_t * allthreads = 0;

	SPIN_AUTOLOCK(&lock) {
		if (0 == allthreads) {
			/* All threads */
			allthreads = tree_new(0, TREE_TREAP);
		}
		if (add) {
			if (map_putpp(allthreads, thread, thread)) {
				kernel_panic("Adding existing thread!");
			}
		} else {
			if (0 == map_removepp(allthreads, thread)) {
				kernel_panic("Removing non-existent thread!");
			}
		}
	}
}

thread_t * thread_fork()
{
	thread_t * this = arch_get_thread();
	thread_t * thread = slab_calloc(threads);

	thread->priority = this->priority;
	thread->process = this->process;
	char buf[32];
	snprintf( buf, sizeof(buf), "Child of %p", this);
	thread_set_name(thread, strndup(buf, sizeof(buf)));

	if (0 == arch_thread_fork(thread)) {
		return 0;
	}

	thread_track(thread, 1);
	thread_resume(thread);

	return thread;
}

void thread_exit(void * retval)
{
	thread_t * this = arch_get_thread();

	this->retval = retval;
	this->state = THREAD_TERMINATED;

	/* Signify to any threads waiting on this thread */
	MONITOR_AUTOLOCK(this->lock) {
		monitor_broadcast(this->lock);
	}

	/* Remove this thread from the set of process threads */
	if (this->process) {
		MONITOR_AUTOLOCK(&this->process->lock) {
			map_removepp(this->process->threads, this);
		}
	}

	this->process = 0;

	/* Remove this thread from the set of all threads */
	thread_track(this, 0);

	/* Schedule the next thread */
	thread_schedule();
}

void * thread_join(thread_t * thread)
{
	void * retval = 0;

	MONITOR_AUTOLOCK(thread->lock) {
		while(thread->state != THREAD_TERMINATED) {
			monitor_wait(thread->lock);
		}
	}
	retval = thread->retval;

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

static GCROOT void ** roots;

#define GCPROFILE 1
void thread_gc()
{
	// thread_cleanlocks();
#if GCPROFILE
	timerspec_t start = timer_uptime();
#endif
	slab_gc_begin();
	slab_gc();
	slab_gc_end();
#if GCPROFILE
	static timerspec_t gctime = 0;
	gctime += (timer_uptime() - start);
#endif
}

void thread_gc_root(void * p)
{
	static int rootcount = 0;

	roots = realloc(roots, sizeof(*roots)*(rootcount+1));
	roots[rootcount++] = p;
}

static void thread_mark(void * p)
{
	thread_t * thread = (thread_t *)p;

	slab_gc_mark(thread->name);
	if (thread->state != THREAD_TERMINATED) {
		/* Mark live state only */
		arch_thread_mark(thread);
		slab_gc_mark_block(thread->tls, sizeof(thread->tls));
		slab_gc_mark(thread->process);
	} else {
		/* Mark dead state only */
		slab_gc_mark(thread->retval);
	}
}

static void thread_finalize(void * p)
{
	thread_t * thread = (thread_t *)p;

	arena_thread_free();
	arch_thread_finalize(thread);
}

void ** thread_backtrace(void ** buffer, int levels)
{
	return arch_thread_backtrace(buffer, levels);
}

void thread_init()
{
	INIT_ONCE();

	/* Craft a new bootstrap thread to replace the static defined thread */
	sync_init();
	arch_thread_init(slab_calloc(threads));
	thread_track(arch_get_thread(), 1);
}

static void thread_test2();
static void thread_test1(rwlock_t * rw)
{
	void ** bt = thread_backtrace(NULL, 15);
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
	void ** bt = thread_backtrace(NULL, 15);
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
