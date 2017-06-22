#include "thread.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>

typedef int tls_key;

#define TLS_MAX 32
typedef struct thread_s {
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
	struct thread_s *prev;
	struct thread_s *next;
} thread_t;

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

typedef struct lock_s {
	int spin;
	void * p;
	int count;
	thread_t * owner;
	thread_t * waiting;
	thread_t * condwaiting;
} lock_t;

static void lock_mark(void * p)
{
	lock_t * lock = p;

	if (lock->count) {
		slab_gc_mark(lock->p);
	}
	slab_gc_mark(lock->owner);
	slab_gc_mark(lock->waiting);
	slab_gc_mark(lock->condwaiting);
}

static slab_type_t locks[1] = {SLAB_TYPE(sizeof(lock_t), lock_mark, 0)};
static map_t * locktable;
static int locktablespin;

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

static thread_t * thread_queue(thread_t * queue, thread_t * thread, tstate state)
{
	if (0 == thread) {
		thread = arch_get_thread();
	}
	thread->state = state;
	LIST_APPEND(queue, thread);
	return queue;
}

static void thread_lock_signal(struct lock_s * lock)
{
	thread_t * resume = lock->waiting;

	if (resume) {
		LIST_DELETE(lock->waiting, resume);
		thread_resume(resume);
	}
}

static void thread_cond_signal(struct lock_s * lock)
{
	thread_t * resume = lock->condwaiting;

	if (resume) {
		LIST_DELETE(lock->condwaiting, resume);
		thread_resume(resume);
	}
}

static void thread_cond_broadcast(struct lock_s * lock)
{
	thread_t * resume = lock->condwaiting;

	while(resume) {
		LIST_DELETE(lock->condwaiting, resume);
		thread_resume(resume);
		resume = lock->condwaiting;
	}
}

static void thread_lock_wait(struct lock_s * lock)
{
	lock->waiting = thread_queue(lock->waiting, 0, THREAD_SLEEPING);
	thread_lock_signal(lock);
	spin_unlock(&lock->spin);
	thread_schedule();
}

static void thread_cond_wait(struct lock_s * lock)
{
	lock->condwaiting = thread_queue(lock->condwaiting, 0, THREAD_SLEEPING);
	thread_lock_signal(lock);
	spin_unlock(&lock->spin);
	thread_schedule();
}

static struct lock_s * thread_lock_get(void * p)
{
	while(1) {
		if (spin_trylock(&locktablespin)) {
			if (0 == locktable) {
				locktable = tree_new(0, TREE_SPLAY);
			}

			lock_t * lock = map_getpp(locktable, p);
			if (0 == lock) {
				lock = slab_calloc(locks);
				lock->p = p;
				map_putpp(locktable, p, lock);
			}
			spin_unlock(&locktablespin);

			while(1) {
				if (spin_trylock(&lock->spin)) {
					return lock;
				} else {
					/* Lock in use by another pointer */
					contended++;
					thread_lock_wait(lock);
				}
			}
		}
	}
}

static int thread_trylock_internal(struct lock_s * lock, void * p)
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

int thread_trylock(void * p)
{
	struct lock_s * lock = thread_lock_get(p);

	return thread_trylock_internal(lock, p);
}

void thread_lock(void * p)
{
	while(1) {
		struct lock_s * lock = thread_lock_get(p);

		if (thread_trylock_internal(lock, p)) {
			return;
		} else {
			/* Owned by someone else */
			thread_lock_wait(lock);
		}
	}
}

void thread_unlock(void *p)
{
	struct lock_s * lock = thread_lock_get(p);

	if (lock->owner == arch_get_thread()) {
		/* We own the lock, unlock it */
		lock->count--;
		if (0 == lock->count) {
			lock->owner = 0;
			thread_lock_signal(lock);
		}
	} else {
		kernel_panic("Unlocking unowned lock\n");
	}
	spin_unlock(&lock->spin);
}

void thread_signal(void *p)
{
	struct lock_s * lock = thread_lock_get(p);
	if (lock->owner == arch_get_thread()) {
		thread_cond_signal(lock);
	} else {
		kernel_panic("Signalling unowned lock\n");
	}
	spin_unlock(&lock->spin);
}

void thread_broadcast(void *p)
{
	struct lock_s * lock = thread_lock_get(p);
	if (lock->owner == arch_get_thread()) {
		thread_cond_broadcast(lock);
	} else {
		kernel_panic("Signalling unowned lock\n");
	}
	spin_unlock(&lock->spin);
}

void thread_wait(void *p)
{
	struct lock_s * lock = thread_lock_get(p);

	if (lock->owner == arch_get_thread()) {
		int count = lock->count;
		lock->count = 0;
		lock->owner = 0;
		thread_cond_wait(lock);
		while(1) {
			lock = thread_lock_get(p);

			if (thread_trylock_internal(lock, p)) {
				/* We have the lock again, restore count */
				lock->count = count;
				return;
			} else {
				/* Owned by someone else */
				thread_lock_wait(lock);
			}
		}
	} else {
		kernel_panic("Unlocking unowned lock\n");
	}
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
			if (arch_get_thread() != next) {
				/* Changing threads */
				arch_thread_switch(next);
			}
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

void thread_init()
{
	INIT_ONCE();

	/* Craft a new bootstrap thread to replace the static defined thread */
	arch_thread_init(slab_alloc(threads));
}

static void thread_test2();
static void thread_test1()
{
	kernel_printk("thread_test1\n");
}

static void thread_test2()
{
	kernel_printk("thread_test2\n");
}

void thread_test()
{
	thread_t * thread1;

	thread1 = thread_fork();
	if (thread1) {
		thread_join(thread1);
	} else {
		thread_t * thread2 = thread_fork();
		if (thread2) {
			thread_test1();
			thread_join(thread2);
			thread_exit(0);
		} else {
			thread_test2();
			thread_exit(0);
		}
	}
}
