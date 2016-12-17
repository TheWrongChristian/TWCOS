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

	/* Run state */
	tstate state;

	/* Queue */
	struct thread_s *prev;
	struct thread_s *next;
} thread_t;

enum tstate { THREAD_NEW, THREAD_RUNNABLE, THREAD_RUNNING, THREAD_SLEEPING };

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

static slab_type_t threads[1];

#define LOCK_COUNT 64
static struct lock_s {
	int spin;
	void * p;
	int count;
	thread_t * owner;
	thread_t * waiting;
	thread_t * condwaiting;
} locks[LOCK_COUNT];

static int lockcount;
static int contended;

int spin_trylock(int * l)
{
	return arch_spin_trylock(l);
}

void spin_unlock(int * l)
{
	arch_spin_unlock(l);
}

static thread_t * thread_queue(thread_t * queue, tstate state)
{
	thread_t * self = arch_get_thread();
	self->state = state;
	LIST_APPEND(queue, self);
	return queue;
}

static void thread_lock_wait(struct lock_s * lock)
{
	thread_queue(lock->waiting, THREAD_SLEEPING);
	spin_unlock(&lock->spin);
	thread_schedule();
}

static void thread_lock_signal(struct lock_s * lock)
{
}

static struct lock_s * thread_lock_hash(void * p)
{
	int hash = ((ptri)p * 997) & (LOCK_COUNT-1);
	int nexthash = hash;
	struct lock_s * lock = locks+hash;

	/*  */
	while(1) {
		if (spin_trylock(&lock->spin)) {
			/* Got the lock, test if it's in use */
			if (0 == lock->count) {
				lock->p = p;
				return lock;
			} else if (p == lock->p) {
				return lock;
			} else {
				/* Lock in use by another pointer */
				contended++;
				thread_lock_wait(lock);
			}
		}
		/* FIXME: yield? */
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
	struct lock_s * lock = thread_lock_hash(p);

	return thread_trylock_internal(lock, p);
}

void thread_lock(void * p)
{
	while(1) {
		struct lock_s * lock = thread_lock_hash(p);

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
	struct lock_s * lock = thread_lock_hash(p);

	if (lock->owner == arch_get_thread()) {
		/* We own the lock, unlock it */
		lock->count--;
		if (0 == lock->count && lock->waiting) {
			thread_lock_signal(lock);
		}
	} else {
		kernel_panic("Unlocking unowned lock\n");
	}
	spin_unlock(&lock->spin);
}

void thread_signal(void *p)
{
	struct lock_s * lock = thread_lock_hash(p);
	if (lock->owner == arch_get_thread()) {
		if (lock->condwaiting) {
			thread_t * wake = lock->condwaiting;
			LIST_DELETE(lock->condwaiting, wake);
		}
	} else {
		kernel_panic("Signalling unowned lock\n");
	}
	spin_unlock(&lock->spin);
}

void thread_wait(void *p)
{
	struct lock_s * lock = thread_lock_hash(p);

	if (lock->owner == arch_get_thread()) {
		int count = lock->count;
		lock->count = 0;
		thread_lock_wait(lock);
		while(1) {
			lock = thread_lock_hash(p);

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
static thread_t * queue;
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
	scheduler_lock();
	queue = thread_queue(queue, THREAD_RUNNABLE);
	scheduler_unlock();
	thread_schedule();
}

void thread_schedule()
{
	scheduler_lock();
	if (0 == queue) {
		kernel_panic("Empty run queue!\n");
	} else {
		thread_t * next = queue;
		LIST_DELETE(queue, next);
		scheduler_unlock();
		if (arch_get_thread() != next) {
			/* Changing threads */
			arch_thread_switch(next);
		}
	}
}

thread_t * thread_fork()
{
	thread_t * thread = slab_alloc(threads);

	if (arch_thread_fork(thread)) {
		return 0;
	}

	scheduler_lock();
	LIST_APPEND(queue, thread);
	scheduler_unlock();

	return thread;
}

void thread_init()
{
	slab_type_create(threads, sizeof(thread_t));
}

void thread_test()
{
	static thread_t * old;
	thread_t * new_thread;

	old = arch_get_thread();
	new_thread = thread_fork();
	if (new_thread) {
		thread_yield();
		kernel_printk("Back to main thread 1\n");
		thread_yield();
		kernel_printk("Back to main thread 2\n");
	} else {
		kernel_printk("In test thread 1\n");
		thread_yield();
		kernel_printk("In test thread 2\n");
		thread_yield();
	}
}
