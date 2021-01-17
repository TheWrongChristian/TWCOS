#include "sync.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>

typedef int spin_t;

struct monitor_t {
	interrupt_monitor_t lock[1];
	thread_t * owner;
	int count;
};

struct rwlock_t {
	monitor_t lock[1];
	int readcount;
	thread_t * writer;
};

struct interrupt_monitor_t {
	spin_t spin;
	thread_t * waiting;
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
#define SPIN_AUTOLOCK(lock) spin_t AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =spin_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#if 0
#define MUTEX_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =mutex_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#endif
#define MUTEX_AUTOLOCK(lock) MONITOR_AUTOLOCK(lock)
#define MONITOR_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =monitor_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#define INTERRUPT_MONITOR_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =interrupt_monitor_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#define READER_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =rwlock_autolock(lock, AUTOLOCK_VAR(__LINE__), 0)))
#define WRITER_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =rwlock_autolock(lock, AUTOLOCK_VAR(__LINE__), 1)))

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

static void monitor_mark(void * p)
{
	monitor_t * lock = p;

	slab_gc_mark(lock->lock->waiting);
	slab_gc_mark(lock->owner);
}

static slab_type_t monitors[1] = {SLAB_TYPE(sizeof(monitor_t), monitor_mark, 0)};
static GCROOT map_t * locktable;
static mutex_t locktablelock;

static int contended;

int spin_trylock(spin_t * l)
{
	return arch_spin_trylock(l);
}

void spin_unlock(spin_t * l)
{
	arch_spin_unlock(l);
}

void spin_lock(spin_t * l)
{
	arch_spin_lock(l);
}

#if 0
int mutex_autolock(mutex_t * lock, int state)
{
        if (state) {
                mutex_unlock(lock);
                state = 0;
        } else {
                mutex_lock(lock);
                state = 1;
        }

        return state;
} 
#endif

int spin_autolock(spin_t * lock, int state)
{
        if (state) {
                spin_unlock(lock);
                state = 0;
        } else {
                spin_lock(lock);
                state = 1;
        }

        return state;
} 

int monitor_autolock(monitor_t * lock, int state)
{
        if (state) {
                monitor_leave(lock);
                state = 0;
        } else {
                monitor_enter(lock);
                state = 1;
        }

        return state;
} 

int interrupt_monitor_autolock(interrupt_monitor_t * lock, int state)
{
        if (state) {
                interrupt_monitor_leave(lock);
                state = 0;
        } else {
                interrupt_monitor_enter(lock);
                state = 1;
        }

        return state;
} 

int rwlock_autolock(rwlock_t * lock, int state, int write)
{
        if (state) {
		rwlock_unlock(lock);
                state = 0;
        } else {
		if (write) {
			rwlock_write(lock);
		} else {
			rwlock_read(lock);
		}
                state = 1;
        }

        return state;
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

monitor_t * monitor_create()
{
	return slab_calloc(monitors);
}

void monitor_enter(monitor_t * monitor)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		thread_t * thread = arch_get_thread();

		if (0 == monitor->owner) {
			/* Unlocked case */
			monitor->owner = thread;
		} else if (thread != monitor->owner) {
			/* Locked by someone else */
			while (0 != monitor->owner) {
				interrupt_monitor_wait(monitor->lock);
			}
		}
		monitor->count++;
	}
}

void monitor_leave(monitor_t * monitor)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		thread_t * thread = arch_get_thread();

		monitor->count--;
                if (0 == monitor->count) {
                        monitor->owner = 0;
                        interrupt_monitor_signal(monitor->lock);
                }
	}
}

void monitor_wait_timeout(monitor_t * monitor, timerspec_t timeout)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		assert(monitor->owner == arch_get_thread());
		int count = monitor->count;
		monitor->count = 0;
		monitor->owner = 0;
		interrupt_monitor_wait_timeout(monitor->lock, timeout);
		monitor->owner = arch_get_thread();
		monitor->count = count;
	}
}

void monitor_wait(monitor_t * monitor)
{
	monitor_wait_timeout(monitor, 0);
}

void monitor_signal(monitor_t * monitor)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		interrupt_monitor_signal(monitor->lock);
	}
}

void monitor_broadcast(monitor_t * monitor)
{
	INTERRUPT_MONITOR_AUTOLOCK(monitor->lock) {
		assert(monitor->owner == arch_get_thread());
		interrupt_monitor_broadcast(monitor->lock);
	}
}

void interrupt_monitor_enter(interrupt_monitor_t * monitor)
{
	spin_lock(&monitor->spin);
}

void interrupt_monitor_leave(interrupt_monitor_t * monitor)
{
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

void interrupt_monitor_wait_timeout(interrupt_monitor_t * monitor, timerspec_t timeout)
{
	interrupt_monitor_wait_timeout_t timeout_thread = { monitor, arch_get_thread() };
	timer_event_t * timer;
	if (timeout) {
		timer = timer_add(timeout, interrupt_monitor_wait_timeout_thread, &timeout_thread);
	} else {
		timer = 0;
	}
	monitor->waiting = thread_queue(monitor->waiting, 0, THREAD_SLEEPING);
	spin_unlock(&monitor->spin);
	thread_schedule();

	/* Check for timeout */
	if (timer) {
		timer_delete(timer);
		if (thread_interrupted()) {
			KTHROW(TimeoutException, "Timeout");
		}
	}
	spin_lock(&monitor->spin);
}

void interrupt_monitor_wait(interrupt_monitor_t * monitor)
{
	interrupt_monitor_wait_timeout(monitor, 0);
}

void interrupt_monitor_signal(interrupt_monitor_t * monitor)
{
	thread_t * resume = monitor->waiting;

	if (resume) {
		LIST_DELETE(monitor->waiting, resume);
		thread_resume(resume);
	}
}

void interrupt_monitor_broadcast(interrupt_monitor_t * monitor)
{
	while(monitor->waiting) {
		interrupt_monitor_signal(monitor);
	}
}

static interrupt_monitor_t locks[32] = {0};
static void interrupt_monitor_irq_trigger(int irq)
{
	INTERRUPT_MONITOR_AUTOLOCK(locks+irq) {
                interrupt_monitor_broadcast(locks+irq);
        }
}

#if 0
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

void sync_init()
{
	INIT_ONCE();
	locktable = tree_new(0, TREE_SPLAY);
}
