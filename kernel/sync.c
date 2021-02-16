#include "sync.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

typedef volatile atomic_flag spin_t;

/**
 * Monitor synchronisation primitive
 *
 * Monitors is a combined mutex/condition variable, similar to Java monitors.
 * \see https://en.wikipedia.org/wiki/Monitor_(synchronization)
 *
 * To lock a monitor, you \ref monitor_enter the monitor. Whilst in the monitor,
 * you own the monitor to the exclusion of other threads.
 *
 * To unlock a monitor, you \ref monitor_leave the monitor. After you leave the monitor,
 * you no longer own the monitor and cannot access any data protected by the monitor.
 *
 * Whilst in the monitor, you can \ref monitor_wait for a condition, putting the thread
 * to sleep and leaving the monitor.
 *
 * Whilst in the monitor, you can \ref monitor_signal or \ref monitor_broadcast the monitor
 * to indicate some condition is now true, waking up thread(s) in \ref monitor_wait.
 */
struct monitor_t {
	interrupt_monitor_t lock[1];
	thread_t * volatile owner;
	volatile int count;
};

/**
 * Reader writer lock synchronisation primitive.
 *
 * Reader writer locks allow either multiple readers or a single writer
 * enter the critical section protected by the lock.
 *
 * Readers can only read the data so protected, but a writer can modify
 * the data safe in the knowledge that no-one else is reading nor writing
 * the data.
 *
 * Read lock is acquired using \ref rwlock_read, and can be escalated
 * to a write lock using \ref rwlock_escalate.
 *
 * A write lock is acquired using using \ref rwlock_write.
 *
 * Either lock type is released using \ref rwlock_unlock.
 */
struct rwlock_t {
	monitor_t lock[1];
	int readcount;
	thread_t * volatile writer;
};

/**
 * Interrupt safe monitor synchronisation primitive
 *
 * Monitors is a combined mutex/condition variable, similar to Java monitors.
 * \see https://en.wikipedia.org/wiki/Monitor_(synchronization)
 *
 * To lock a interrupt monitor, you \ref interrupt_monitor_enter the monitor. Whilst in the monitor,
 * you own the monitor to the exclusion of other threads.
 *
 * To unlock a monitor, you \ref interrupt_monitor_leave the monitor. After you leave the monitor,
 * you no longer own the monitor and cannot access any data protected by the monitor.
 *
 * Whilst in the monitor, you can \ref interrupt_monitor_wait for a condition, putting the thread
 * to sleep and leaving the monitor.
 *
 * Whilst in the monitor, you can \ref interrupt_monitor_signal or \ref interrupt_monitor_broadcast the monitor
 * to indicate some condition is now true, waking up thread(s) in \ref interrupt_monitor_wait.
 */
struct interrupt_monitor_t {
	spin_t spin;
	int onerror;
	thread_t * volatile owner;
	thread_t * volatile waiting;
	timer_event_t timer[1];
};

#define INIT_ONCE() \
	do { \
		static int inited = 0; \
		if (inited) { \
			return; \
		} \
		inited = 1; \
	} while(0)

typedef monitor_t mutex_t;

#define AUTOLOCK_CONCAT(a, b) a ## b
#define AUTOLOCK_VAR(line) AUTOLOCK_CONCAT(s,line)
#define AUTOLOCK_DO(lock, locklock, unlocklock) int AUTOLOCK_VAR(__LINE__) = 1; for(locklock(lock); AUTOLOCK_VAR(__LINE__)--; unlocklock(lock))

/**
 * Wrap the following statement (or block statement) by the interrupt monitor.
 * \arg lock The lock.
 */
#define INTERRUPT_MONITOR_AUTOLOCK(lock) AUTOLOCK_DO(lock, interrupt_monitor_enter, interrupt_monitor_leave)
/**
 * Wrap the following statement (or block statement) by the monitor.
 * \arg lock The lock.
 */
#define MONITOR_AUTOLOCK(lock) AUTOLOCK_DO(lock, monitor_enter, monitor_leave)
/**
 * Wrap the following statement (or block statement) by the spin lock.
 * \arg lock The lock.
 */
#define SPIN_AUTOLOCK(lock) AUTOLOCK_DO(lock, spin_lock, spin_unlock)
/**
 * Wrap the following statement (or block statement) by the mutex.
 * \arg lock The lock.
 */
#define MUTEX_AUTOLOCK(lock) MONITOR_AUTOLOCK(lock)
/**
 * Wrap the following statement (or block statement) by the reader lock.
 * \arg lock The lock.
 */
#define READER_AUTOLOCK(lock) AUTOLOCK_DO(lock, rwlock_read, rwlock_unlock)
/**
 * Wrap the following statement (or block statement) by the writer lock.
 * \arg lock The lock.
 */
#define WRITER_AUTOLOCK(lock) AUTOLOCK_DO(lock, rwlock_write, rwlock_unlock)

#endif

exception_def TimeoutException = { "TimeoutException", &Exception };

#if 0
static void mutex_mark(void * p)
{
	mutex_t * lock = p;

	slab_gc_mark(lock->owner);
	slab_gc_mark(lock->waiting);
}

static slab_type_t mutexes[1] = {SLAB_TYPE(sizeof(mutex_t), mutex_mark, 0)};
#endif

#if 0
static void monitor_mark(void * p)
{
	monitor_t * lock = p;

	slab_gc_mark(lock->lock->waiting);
	slab_gc_mark(lock->owner);
}
#endif

#if 0
static slab_type_t monitors[1] = {SLAB_TYPE(sizeof(monitor_t), 0, 0)};
static GCROOT map_t * locktable;
static mutex_t locktablelock;
#endif

/**
 * Try to lock the given spin lock.
 *
 * \arg l The lock
 * \return 1 if the lock was acquired, 0 if the lock is already locked.
 */
int spin_trylock(spin_t * l)
{
	arch_interrupt_block();
	if (atomic_flag_test_and_set(l)) {
                arch_interrupt_unblock();
                return 0;
        }
        return 1;
}

/**
 * Unlock the given spin lock.
 *
 * \arg l The lock
 */
void spin_unlock(spin_t * l)
{
	atomic_flag_clear(l);
	arch_interrupt_unblock();
}

/**
 * Lock the given spin lock.
 *
 * \arg l The lock
 */
void spin_lock(spin_t * l)
{
	while(0 == spin_trylock(l)) {
		arch_pause();
	}
}

#if 0
static void thread_lock_signal(mutex_t * lock)
{
	thread_t * resume = lock->waiting;

	if (resume) {
		LIST_DELETE(lock->waiting, resume);
		thread_resume(resume);
	}
}

static void thread_lock_wait(mutex_t * lock)
{
	lock->waiting = thread_queue(lock->waiting, 0, THREAD_SLEEPING);
	/* thread_lock_signal(lock); */
	spin_unlock(&lock->spin);
	thread_schedule();
	spin_lock(&lock->spin);
}

mutex_t * mutex_create()
{
	return slab_calloc(mutexes);
}
#endif

void mutex_lock(mutex_t * lock)
{
	monitor_enter(lock);
}

void mutex_unlock(mutex_t * lock)
{
	monitor_leave(lock);
}

/**
 * Enter (lock) the monitor.
 *
 * \arg monitor The monitor lock.
 */
void monitor_enter(monitor_t * monitor)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		thread_t * thread = arch_get_thread();

		while(monitor->owner && thread != monitor->owner) {
			/* Locked by someone else */
			interrupt_monitor_wait(monitor->lock);
		}
		monitor->owner = thread;
		monitor->count++;
	}
	assert(monitor->owner == arch_get_thread());
}

/**
 * Leave (unlock) the monitor.
 *
 * \arg monitor The monitor lock.
 */
void monitor_leave(monitor_t * monitor)
{
	assert(monitor->owner == arch_get_thread());
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		monitor->count--;
                if (0 == monitor->count) {
                        monitor->owner = 0;
                        interrupt_monitor_signal(monitor->lock);
                }
	}
}

/**
 * Wait for a condition.
 *
 * \arg monitor The monitor lock.
 * \arg timeout Timeout in microseconds.
 * \throw TimeoutException if the condition doesn't happen with the timeout
 */
void monitor_wait_timeout(monitor_t * monitor, timerspec_t timeout)
{
	assert(monitor->owner == arch_get_thread());
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		int count = monitor->count;
		monitor->count = 0;
		monitor->owner = 0;
		interrupt_monitor_wait_timeout(monitor->lock, timeout);
		monitor->owner = arch_get_thread();
		monitor->count = count;
	}
}

/**
 * Wait for a condition.
 *
 * \arg monitor The monitor lock.
 */
void monitor_wait(monitor_t * monitor)
{
	monitor_wait_timeout(monitor, 0);
}

/**
 * Signal a condition, waking up a waiting thread.
 *
 * \arg monitor The monitor lock.
 */
void monitor_signal(monitor_t * monitor)
{
	assert(monitor->owner == arch_get_thread());
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		interrupt_monitor_signal(monitor->lock);
	}
}

/**
 * Signal a condition, waking up all waiting threads.
 *
 * \arg monitor The monitor lock.
 */
void monitor_broadcast(monitor_t * monitor)
{
	assert(monitor->owner == arch_get_thread());
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		interrupt_monitor_broadcast(monitor->lock);
	}
}

static int interrupt_monitor_deadlock_visit_monitor(map_t * visited, interrupt_monitor_t * monitor);
static int interrupt_monitor_deadlock_visit_thread(map_t * visited, thread_t * thread)
{
	if (thread == map_putpp(visited, thread, thread)) {
		return 1;
	}

	return interrupt_monitor_deadlock_visit_monitor(visited, thread->waitingfor);
}

static int interrupt_monitor_deadlock_visit_monitor(map_t * visited, interrupt_monitor_t * monitor)
{
	if (monitor == map_putpp(visited, monitor, monitor)) {
		return 1;
	}

	thread_t * thread = monitor->waiting;
	while(thread) {
		if (interrupt_monitor_deadlock_visit_thread(visited, thread)) {
			return 1;
		}
		LIST_NEXT(monitor->waiting, thread);
	}

	return interrupt_monitor_deadlock_visit_thread(visited, monitor->owner);
}

static void interrupt_monitor_owned(interrupt_monitor_t * monitor)
{
	if(monitor->owner != arch_get_thread()) {
		kernel_printk("owner = %p, current = %p\n", monitor->owner, arch_get_thread());
		kernel_backtrace(logger_debug);
	}
}

static INLINE_FLATTEN void interrupt_monitor_leave_dtor(void * p)
{
	interrupt_monitor_leave(p);
}

/**
 * Enter (lock) the monitor.
 *
 * \arg monitor The monitor lock.
 */
void interrupt_monitor_enter(interrupt_monitor_t * monitor)
{
	interrupt_monitor_t * waitingfor = arch_get_thread()->waitingfor;
	arch_get_thread()->waitingfor = monitor;
	int attempts = 0;
	while(0 == spin_trylock(&monitor->spin)) {
		attempts++;
		if (0 == (attempts & 0xffff)) {
			/* Try to detect deadlock */
			map_t * visited = treap_new(NULL);
			if (interrupt_monitor_deadlock_visit_thread(visited, arch_get_thread())) {
				thread_yield();
			}
		}
	}
	arch_get_thread()->waitingfor = waitingfor;
	monitor->owner = arch_get_thread();
	monitor->onerror = exception_onerror(interrupt_monitor_leave_dtor, monitor);
	interrupt_monitor_owned(monitor);
}

static void interrupt_monitor_leave_and_schedule(interrupt_monitor_t * monitor)
{
	scheduler_lock();	
	monitor->waiting = thread_queue(monitor->waiting, 0, THREAD_SLEEPING);
	interrupt_monitor_leave(monitor);
	thread_schedule();
}

/**
 * Leave (unlock) the monitor.
 *
 * \arg monitor The monitor lock.
 */
void interrupt_monitor_leave(interrupt_monitor_t * monitor)
{
	interrupt_monitor_owned(monitor);
	monitor->owner = 0;
	exception_onerror_pop(monitor->onerror);
	spin_unlock(&monitor->spin);
}

typedef struct interrupt_monitor_wait_timeout_t {
	interrupt_monitor_t * monitor;
	thread_t * thread;
} interrupt_monitor_wait_timeout_t;


static void interrupt_monitor_wait_timeout_thread(void * p)
{
	interrupt_monitor_wait_timeout_t * timeout = p;

	INTERRUPT_MONITOR_AUTOLOCK(timeout->monitor) {
		thread_t * thread = timeout->monitor->waiting;

		while(thread) {
			if (thread == timeout->thread) {
				LIST_DELETE(timeout->monitor->waiting, thread);
				thread_interrupt(thread);
				thread_resume(thread);
				thread = 0;
			} else {
				LIST_NEXT(timeout->monitor->waiting, thread);
			}
		}
	}
}

/**
 * Wait for a condition.
 *
 * \arg monitor The monitor lock.
 * \arg timeout Timeout in microseconds.
 * \throw TimeoutException if the condition doesn't happen with the timeout
 */
void interrupt_monitor_wait_timeout(interrupt_monitor_t * monitor, timerspec_t timeout)
{
	interrupt_monitor_owned(monitor);
	interrupt_monitor_wait_timeout_t timeout_thread = { monitor, arch_get_thread() };
	if (timeout) {
		timer_set(monitor->timer, timeout, interrupt_monitor_wait_timeout_thread, &timeout_thread);
	}
	KTRY {
		interrupt_monitor_leave_and_schedule(monitor);

		/* Check for timeout */
		if (timeout) {
			timer_delete(monitor->timer);
			if (thread_interrupted()) {
				KTHROW(TimeoutException, "Timeout");
			}
		}
	} KFINALLY {
		interrupt_monitor_enter(monitor);
	}
}

/**
 * Wait for a condition.
 *
 * \arg monitor The monitor lock.
 */
void interrupt_monitor_wait(interrupt_monitor_t * monitor)
{
	interrupt_monitor_owned(monitor);
	interrupt_monitor_wait_timeout(monitor, 0);
}

/**
 * Signal a condition, waking up a waiting thread.
 *
 * \arg monitor The monitor lock.
 */
void interrupt_monitor_signal(interrupt_monitor_t * monitor)
{
	interrupt_monitor_owned(monitor);

	thread_t * resume = monitor->waiting;

	if (resume) {
		LIST_DELETE(monitor->waiting, resume);
		thread_resume(resume);
	}
}

/**
 * Signal a condition, waking up all waiting threads.
 *
 * \arg monitor The monitor lock.
 */
void interrupt_monitor_broadcast(interrupt_monitor_t * monitor)
{
	while(monitor->waiting) {
		interrupt_monitor_signal(monitor);
	}
}

#if 0
static interrupt_monitor_t locks[32] = {0};
static void interrupt_monitor_irq_trigger(int irq)
{
	INTERRUPT_MONITOR_AUTOLOCK(locks+irq) {
                interrupt_monitor_broadcast(locks+irq);
        }
}

static monitor_t * thread_monitor_get(void * p)
{
	mutex_lock(&locktablelock);
	if (0 == locktable) {
		locktable = tree_new(0, TREE_SPLAY);
	}

	monitor_t * lock = map_getpp(locktable, p);
	if (0 == lock) {
		lock = slab_calloc(monitors);
		map_putpp(locktable, p, lock);
	}

	mutex_unlock(&locktablelock);

	return lock;
}

static int thread_trylock_internal(mutex_t * lock)
{
	if (0 == lock->count) {
		/* Not in use, mark it as used and locked */
		lock->count = 1;
		lock->owner = arch_get_thread();
		spin_unlock(&lock->spin);
		return 1;
	} else if (lock->owner == arch_get_thread()) {
		/* Recursive lock, increase count */
		lock->count++;
		spin_unlock(&lock->spin);
		return 1;
	}

	return 0;
}
#endif

/**
 * Lock a read/write lock in read mode.
 *
 * \arg lock The lock.
 */
void rwlock_read(rwlock_t * lock)
{
	MONITOR_AUTOLOCK(lock->lock) {
		if (arch_get_thread() == lock->writer) {
			/* We already have a write lock, give it up */
			lock->writer = 0;
		} else {
			while(lock->writer) {
				monitor_wait(lock->lock);
			}
		}
		lock->readcount++;
	}
}

/**
 * Lock a read/write lock in write mode.
 *
 * \arg lock The lock.
 */
void rwlock_write(rwlock_t * lock)
{
	thread_t * thread = arch_get_thread();
	MONITOR_AUTOLOCK(lock->lock) {
		/* Not already a reader, wait until no writer or readers */
		while(lock->readcount || lock->writer) {
			monitor_wait(lock->lock);
		}
		lock->writer = thread;
	}
}

/**
 * Escalate a read/write lock in read mode to write mode.
 *
 * \arg lock The lock.
 */
void rwlock_escalate(rwlock_t * lock)
{
	thread_t * thread = arch_get_thread();
	MONITOR_AUTOLOCK(lock->lock) {
		while(lock->readcount > 1 || lock->writer) {
			monitor_wait(lock->lock);
		}

		lock->readcount = 0;
		lock->writer = thread;
	}
}

/**
 * Unlock a read/write lock.
 *
 * \arg lock The lock.
 */
void rwlock_unlock(rwlock_t * lock)
{
	thread_t * thread = arch_get_thread();
	MONITOR_AUTOLOCK(lock->lock) {
		if (thread == lock->writer) {
			lock->writer = 0;
		} else {
			lock->readcount--;
		}
		monitor_broadcast(lock->lock);
	}
}

#if 0
int thread_tryplock(void * p)
{
	mutex_t * lock = thread_monitor_get(p);

	return thread_trylock_internal(lock);
}

void thread_lock(void * p)
{
	monitor_t * lock = thread_monitor_get(p);

	monitor_enter(lock);
}

void thread_unlock(void *p)
{
	monitor_t * lock = thread_monitor_get(p);

	monitor_leave(lock);
}

void thread_signal(void *p)
{
	monitor_t * lock = thread_monitor_get(p);

	monitor_signal(lock);
}

void thread_broadcast(void *p)
{
	monitor_t * lock = thread_monitor_get(p);

	monitor_broadcast(lock);
}

void thread_wait(void *p)
{
	monitor_t * lock = thread_monitor_get(p);

	monitor_wait(lock);
}
#endif

static void sync_deadlock_test()
{
	static interrupt_monitor_t test[2] = {0};

	thread_t * thread = thread_fork();
	if (thread) {
		INTERRUPT_MONITOR_AUTOLOCK(test) {
			interrupt_monitor_wait(test);
			INTERRUPT_MONITOR_AUTOLOCK(test+1) {
			}
		}
	} else {
		INTERRUPT_MONITOR_AUTOLOCK(test+1) {
			INTERRUPT_MONITOR_AUTOLOCK(test) {
				interrupt_monitor_signal(test);
				thread_yield();
			}
		}
		thread_exit(NULL);
	}
	thread_join(thread);
}

static void monitor_test()
{
	static monitor_t test[1];
	static int running = 1;
	thread_t * thread = thread_fork();
	if (thread) {
		for(int i=0; i<1000000; i++) {
			MONITOR_AUTOLOCK(test) {
				monitor_signal(test);
			}
			thread_yield();
		}
		MONITOR_AUTOLOCK(test) {
			running = 0;
			monitor_signal(test);
		}
		thread_yield();
		thread_join(thread);
	} else {
		MONITOR_AUTOLOCK(test) {
			while(running) {
				monitor_wait(test);
			}
		}
	}
}

void sync_test()
{
	sync_deadlock_test();
	monitor_test();
}
