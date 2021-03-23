#include "thread.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>
#include <stdnoreturn.h>

typedef int tls_key;

#define TLS_MAX 32

/**
 * \brief Representation of a kernel thread.
 */
struct thread_t {
	/* Runtime data */

	/** Thread local storage pointers */
	void * tls[TLS_MAX];
	/** Architecture dependent context */
	arch_context_t context;
	/** Process associated with this thread */
	process_t * process;

	/* Thread CPU usage */
	int acct;
	struct {
		timerspec_t tstart;
		timerspec_t tlen;
	} accts[64];
	timerspec_t period;
	timerspec_t usage;
	timerspec_t preempt;

	/** Current thread state */
	tstate state;
	/** Thread priority */
	tpriority priority;
	/** Set if the thread has been interrupted */
	int interrupted;
	/** If set, indicates which interrupt_monitor_t we're waiting for */
	interrupt_monitor_t * waitingfor;
	/** If set, indicates which interrupt_monitor_t we're sleeping on */
	interrupt_monitor_t * sleepingon;

	/** Thread name */
	char * name;

	/** Thread lock */
	monitor_t lock[1];

	/** Return value for thread_join */
	void * retval;

	/** Thread queue next thread */
	thread_t *next;
	/** Thread queue prev thread */
	thread_t *prev;
};

/** Thread state */
enum tstate {
	/** Thread is new */
	THREAD_NEW,
	/** Thread is runnable */
	THREAD_RUNNABLE,
	/** Thread is running */
	THREAD_RUNNING,
	/** Thread is sleeping */
	THREAD_SLEEPING,
	/** Thread is terminated */
	THREAD_TERMINATED
};

/** Thread priority */
enum tpriority {
	/** Thread interrupt priority */
	THREAD_INTERRUPT = 0,
	/** Thread normal priority */
	THREAD_NORMAL,
	/** Thread idle priority */
	THREAD_IDLE,
	/** Thread interrupt priority count */
	THREAD_PRIORITIES
};

#ifndef barrier
#define barrier() asm volatile("": : :"memory")
#endif

#endif

int preempt;
static tls_key tls_next = 1;

/** 
 * Get the next TLS key
 * \return Next key
 */
int tls_get_key()
{
	return arch_atomic_postinc(&tls_next);
}

/**
 * Set the thread local data given by the key.
 *
 * \arg key TLS key retrieved using \ref tls_get_key
 * \arg p Pointer value
 */
void tls_set(int key, void * p)
{
	thread_t * thread = arch_get_thread();

	check_not_null(thread, "Unable to get thread");
	check_int_bounds(key, 1, TLS_MAX-1, "TLS key out of bounds");

	thread->tls[key] = p;
}

/**
 * Get the thread local data given by the key.
 *
 * \arg key TLS key retrieved using \ref tls_get_key
 * \return Pointer value
 */
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

static spin_t queuelock;

/**
 * Lock scheduler
 */
void scheduler_lock()
{
	spin_lock(&queuelock);
}

static void scheduler_unlock()
{
	spin_unlock(&queuelock);
}

/**
 * Put the given thread on the head of the given queue, in the given state.
 * \arg queue Current queue
 * \arg thread Thread to queue
 * \arg state Thread state
 * \return New queue head
 */
thread_t * thread_prequeue(thread_t * queue, thread_t * thread, tstate state)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}
	thread->state = state;
	LIST_PREPEND(queue, thread);
	return queue;
}

/**
 * Put the given thread on tail of the given queue, in the given state.
 * \arg queue Current queue
 * \arg thread Thread to queue
 * \arg state Thread state
 * \return New queue head
 */
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

/**
 * Pre-empt the current thread, putting the thread onto the head of the run queue
 * \return 1 if the thread was pre-empted
 */
int thread_preempt()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_prequeue(queue[priority], this, THREAD_RUNNABLE);
	return thread_schedule();
}

/**
 * Pre-empt the current thread, putting the thread onto the head of the run queue
 * \return 1 if the thread was pre-empted
 */
int thread_yield()
{
	thread_t * this = arch_get_thread();
	tpriority priority = this->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], this, THREAD_RUNNABLE);
	return thread_schedule();
}

/**
 * Mark given thread as interrupted
 */
void thread_interrupt(thread_t * thread)
{
	thread->interrupted = 1;
}

/**
 * Check if the given thread has been interrupted
 * \arg thread Thread to check for interruption. If 0, check current thread
 * \return true (not 0) if interrupted
 */
int thread_isinterrupted(thread_t * thread)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}

	return thread->interrupted;
}

/**
 * Check if the current thread has been interrupted, and reset the interrupt flag
 * \return true (not 0) if interrupted
 */
int thread_interrupted()
{
	thread_t * thread = arch_get_thread();

	if (thread_isinterrupted(thread)) {
		thread->interrupted = 0;
		return 1;
	}

	return 0;
}

/**
 * Resume the given thread
 * \arg thread Thread to resume
 */
void thread_resume(thread_t * thread)
{
	tpriority priority = thread->priority;
	scheduler_lock();
	queue[priority] = thread_queue(queue[priority], thread, THREAD_RUNNABLE);
	scheduler_unlock();
	/* Check for pre-emption */
	if (arch_get_thread()->priority > priority) {
		preempt = 1;
	}
}

/**
 * Schedule the next thread
 * \return 1 if a new thread was scheduled
 */
int thread_schedule()
{
	while(1) {
		int i;
		for(i=0; i<THREAD_PRIORITIES; i++) {
			if (queue[i]) {
				thread_t * current = arch_get_thread();
				thread_t * next = queue[i];
				LIST_DELETE(queue[i], next);
				scheduler_unlock();
				if (arch_get_thread() != next) {
					/* Thread is changing, do accounting and switch to next */
					current->accts[current->acct].tlen = timer_uptime(1) - current->accts[current->acct].tstart;
					current->acct++;
					if (sizeof(current->accts)/sizeof(current->accts[0]) == current->acct) {
						current->acct = 0;
					}
					arch_thread_switch(next);
					current->accts[current->acct].tstart = timer_uptime(1);
					/* By default, preempt after 100ms */
					current->preempt = current->accts[current->acct].tstart + 100000;
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
		scheduler_lock();
	}
}

/**
 * Get the name of the given thread
 * \arg thread Thread, or current thread if 0
 * \return Thread name, if set
 */
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

/**
 * Set the name of the given thread
 * \arg thread Thread, or current thread if 0
 * \arg name New thread name
 */
void thread_set_name(thread_t * thread, char * name)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}

	thread->name = name;
}

static spin_t allthreadslock = ATOMIC_FLAG_INIT;
static GCROOT map_t * allthreads = 0;
static void thread_track(thread_t * thread, int add)
{
	SPIN_AUTOLOCK(&allthreadslock) {
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

/**
 * Fork the current thread, into a new thread
 *
 * Fork the current thread, returning the new thread pointer to the
 * creator thread, and 0 to the created thread.
 *
 * The created thread is a copy of the creator thread, with the same call chain.
 *
 * Automatic variables that contain pointers to locations in the creator stack are
 * transformed to point to the equivalent locations in the created stack.
 * \return thread pointer to the new thread in the creator, or 0 in the new thread
 */
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

/**
 * Exit the current thread.
 *
 * Exit the current thread. This function never returns.
 * \arg retval Return value returned to the caller of \ref thread_join.
 */
noreturn void thread_exit(void * retval)
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
	scheduler_lock();
	thread_schedule();
	kernel_panic("thread_exit: Should never get here\n");
}

/**
 * Get return code from given thread.
 *
 * Get return code from given thread. If the thread has not yet exited,
 * the current thread will sleep waiting for the thread to exit.
 *
 * \arg thread Thread to get return code from.
 * \return Return code as passed to \ref thread_exit
 */
void * thread_join(thread_t * thread)
{
	MONITOR_AUTOLOCK(thread->lock) {
		while(thread->state != THREAD_TERMINATED) {
			monitor_wait(thread->lock);
		}
	}
	return thread->retval;
}

/**
 * Set the priority of the given thread.
 * \arg thread Thread to set priority of, or current thread if 0.
 * \arg priority New thread priority.
 */
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

#if GCPROFILE
static timerspec_t gctime = 0;
#endif

/**
 * Run garbage collection.
 */
void thread_gc()
{
	// thread_cleanlocks();
#if GCPROFILE
	timerspec_t start = timer_uptime(1);
#endif
	slab_gc_begin();
	slab_gc();
	slab_gc_end();
#if GCPROFILE
	static timerspec_t gctime = 0;
	gctime += (timer_uptime(1) - start);
#endif
}

/**
 * Add a new garbage collection root.
 * \arg p Pointer to the new root.
 */
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

/**
 * Generate a function call stack backtrace
 * \arg buffer Pointer to array of pointers.
 * \arg levels Number of pointers in the \ref buffer array.
 * \return buffer
 */
void ** thread_backtrace(void ** buffer, int levels)
{
	return arch_thread_backtrace(buffer, levels);
}

void thread_init()
{
	INIT_ONCE();

	/* Craft a new bootstrap thread to replace the static defined thread */
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

static void thread_update_acct(const void * const p, void * key, void * data)
{
	const timerspec_t *puptime = p;
	thread_t * thread = key;
	if (thread == arch_get_thread()) {
		thread->accts[thread->acct].tlen = (*puptime) - thread->accts[thread->acct].tstart;
		thread->acct++;
		if (sizeof(thread->accts)/sizeof(thread->accts[0]) == thread->acct) {
			thread->acct = 0;
		}
	}
	timerspec_t sum = 0;
	timerspec_t from = (*puptime) - 1000000;
	for(int i=0; i<countof(thread->accts); i++) {
		timerspec_t acctstart = thread->accts[i].tstart;
		timerspec_t acctend = acctstart + thread->accts[i].tlen;
		if (acctstart > from) {
			sum += (acctend - acctstart);
		} else if (acctend > from) {
			sum += (acctend - from);
		}
	}
	thread->usage = sum;

	int percent = 100 * thread->usage / 1000000;
	kernel_printk("%s: %d\n", thread->name ? thread->name : "Unknown", percent);
}

void thread_update_accts()
{
	SPIN_AUTOLOCK(&allthreadslock) {
		timerspec_t uptime = timer_uptime(1);
		if (allthreads) {
			map_walkpp(allthreads, thread_update_acct, &uptime);
		}
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
		static rwlock_t rw[1] = {0};
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
