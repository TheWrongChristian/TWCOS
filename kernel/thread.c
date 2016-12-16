#include "thread.h"


#if INTERFACE
#include <stdint.h>
#include <stddef.h>

typedef int tls_key;

#define TLS_MAX 32
typedef struct {
	void * tls[TLS_MAX];
	arch_context_t context;
} thread_t;


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

void thread_init()
{
	slab_type_create(threads, sizeof(thread_t));
}

thread_t * thread_fork()
{
	thread_t * thread = slab_alloc(threads);

	if (arch_thread_fork(thread)) {
		return 0;
	}

	return thread;
}

void thread_test()
{
	static thread_t * old;
	thread_t * new_thread;

	old = arch_get_thread();
	new_thread = thread_fork();
	if (new_thread) {
		arch_thread_switch(new_thread);
		kernel_printk("Back to main thread 1\n");
		arch_thread_switch(new_thread);
		kernel_printk("Back to main thread 2\n");
	} else {
		kernel_printk("In test thread 1\n");
		arch_thread_switch(old);
		kernel_printk("In test thread 2\n");
		arch_thread_switch(old);
	}
}
