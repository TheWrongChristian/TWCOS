#include "sync.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>

struct mutex_t {
	int spin;
	int count;
	int getting;
	int state;
	thread_t * owner;
	thread_t * waiting;
};

struct monitor_t {
	mutex_t lock[1];
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

#define AUTOLOCK_CONCAT(a, b) a ## b
#define AUTOLOCK_VAR(line) AUTOLOCK_CONCAT(s,line)
#define SPIN_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =spin_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#define MUTEX_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =mutex_autolock(lock, AUTOLOCK_VAR(__LINE__) )))
#define MONITOR_AUTOLOCK(lock) int AUTOLOCK_VAR(__LINE__) = 0; while((AUTOLOCK_VAR(__LINE__) =monitor_autolock(lock, AUTOLOCK_VAR(__LINE__) )))

#endif

static void mutex_mark(void * p)
{
	mutex_t * lock = p;

	slab_gc_mark(lock->owner);
	slab_gc_mark(lock->waiting);
}

static slab_type_t mutexes[1] = {SLAB_TYPE(sizeof(mutex_t), mutex_mark, 0)};


static void monitor_mark(void * p)
{
	monitor_t * lock = p;

	mutex_mark(lock->lock);
	slab_gc_mark(lock->waiting);
}

static slab_type_t monitors[1] = {SLAB_TYPE(sizeof(monitor_t), monitor_mark, 0)};
map_t * locktable;
static mutex_t locktablelock;

static int contended;

int spin_trylock(int * l)
{
	return arch_spin_trylock(l);
}

void spin_unlock(int * l)
{
	arch_spin_unlock(l);
}

void spin_lock(int * l)
{
	arch_spin_lock(l);
}

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

int spin_autolock(int * lock, int state)
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
	thread_lock_signal(lock);
	spin_unlock(&lock->spin);
	thread_schedule();
	spin_lock(&lock->spin);
}

mutex_t * mutex_create()
{
	return slab_calloc(mutexes);
}

void mutex_lock(mutex_t * lock)
{
	spin_lock(&lock->spin);
	thread_t * thread = arch_get_thread();

	if (0 == lock->owner) {
		/* Unlocked case */
		lock->owner = thread;
	} else if (thread != lock->owner) {
		/* Locked by someone else */
		while (0 != lock->owner) {
			thread_lock_wait(lock);
		}
	}
	lock->count++;

	spin_unlock(&lock->spin);
}

void mutex_unlock(mutex_t * lock)
{
	spin_lock(&lock->spin);
	thread_t * thread = arch_get_thread();

	if (thread == lock->owner) {
		lock->count--;
		if (0 == lock->count) {
			lock->owner = 0;
		}
	} else {
		/* FIXME: panic? */
	}

	spin_unlock(&lock->spin);
}

monitor_t * monitor_create()
{
	return slab_calloc(monitors);
}

void monitor_enter(monitor_t * monitor)
{
	mutex_lock(monitor->lock);
}

void monitor_leave(monitor_t * monitor)
{
	mutex_unlock(monitor->lock);
}

void monitor_wait(monitor_t * monitor)
{
	monitor->waiting = thread_queue(monitor->waiting, 0, THREAD_SLEEPING);
	int count = monitor->lock->count;
	mutex_unlock(monitor->lock);
	thread_schedule();
	mutex_lock(monitor->lock);
	monitor->lock->count = count;
}

void monitor_signal(monitor_t * monitor)
{
	thread_t * resume = monitor->waiting;

	LIST_DELETE(monitor->waiting, resume);
	thread_resume(resume);
}

void monitor_broadcast(monitor_t * monitor)
{
	while(monitor->waiting) {
		monitor_signal(monitor);
	}
}

static monitor_t * thread_monitor_get(void * p)
{
	mutex_lock(&locktablelock);
	if (0 == locktable) {
		locktable = tree_new(0, TREE_SPLAY);
		thread_gc_root(locktable);
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

#if 0
int thread_tryplock(void * p)
{
	mutex_t * lock = thread_monitor_get(p);

	return thread_trylock_internal(lock);
}
#endif

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

#if 0
static void thread_cleanlocks_copy(void * p, void * key, void * data)
{
	map_t * newlocktable = (map_t*)p;
	monitor_t * lock = (monitor_t *)data;

	SPIN_AUTOLOCK(&lock->spin) {
		if (lock->owner || lock->waiting || lock->condwaiting || lock->getting) {
			/* lock in use, copy */
			map_putpp(newlocktable, key, data);
		}
	}
}
#endif

void thread_cleanlocks()
{
#if 0
	SPIN_AUTOLOCK(&locktablelock) {
		map_t * newlocktable = tree_new(0, TREE_SPLAY);
		map_walkpp(locktable, thread_cleanlocks_copy, newlocktable);
		locktable = newlocktable;
	}
#endif
}

void sync_init()
{
	INIT_ONCE();
	locktable = tree_new(0, TREE_SPLAY);
	thread_gc_root(locktable);
}
