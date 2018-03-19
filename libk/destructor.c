#include "destructor.h"

#if INTERFACE

#include <stdint.h>

struct dtor_t {
	dtor_t * next;
	void (*dtor)(void * p);
	void * p;
}

#endif

static tls_key dtor_get_key()
{
	static tls_key key = 0;

	if (0 == key) {
		key = tls_get_key();
	}

	return key;
}

dtor_t * dtor_poll_frame()
{
	return tls_get(dtor_get_key());
}

/* dtor_t frame cache */
static int frames_lock;
static dtor_t ** frames = 0;

void dtor_pop(dtor_t * until)
{
	dtor_t * frame = tls_get(dtor_get_key());
	dtor_t * first = frame;
	dtor_t * last = frame;

	if (frame == until) {
		/* Nothing to do */
		return;
	}

	while(frame && until != frame) {
		if (frame->dtor) {
			frame->dtor(frame->p);
			frame->dtor = 0;
		}
		last = frame;
		frame = frame->next;
	}

	/*
	 * By here, frame first will be the head of the
	 * frames we're freeing, and last is the last frame
	 * we're freeing.
	 */
	SPIN_AUTOLOCK(&frames_lock) {
		last->next = frames[0];
		frames[0] = first;
	}

	tls_set(dtor_get_key(), frame);
}

dtor_t * dtor_push(void (*dtor)(void * p), void * p)
{
	dtor_t * frame = 0;
	SPIN_AUTOLOCK(&frames_lock) {
		if (0 == frames) {
			frames = calloc(1, sizeof(*frames));
			thread_gc_root(frames);
		}
		if (frames[0]) {
			frame = frames[0];
			frames[0] = frame->next;
		} else {
			frame = calloc(1, sizeof(*frame));
		}
	}

	frame->next = tls_get(dtor_get_key());
	frame->dtor = dtor;
	frame->p = p;

	tls_set(dtor_get_key(), frame);

	return frame->next;
}

void dtor_remove( void (*dtor)(void * p), void * p )
{
	dtor_t * frame = tls_get(dtor_get_key());

	while(frame) {
		if (dtor == frame->dtor && p == frame->p) {
			frame->dtor = 0;
			return;
		}
		frame = frame->next;
	}
}

void dtor_test()
{
	dtor_t * p1 = dtor_push(kernel_printk, "p1");
	dtor_push(kernel_printk, "p2");
	dtor_push(kernel_printk, "p3");
	dtor_push(kernel_printk, "p4");
	dtor_t * p5 = dtor_push(kernel_printk, "p5");
	dtor_push(kernel_printk, "p6");
	dtor_push(kernel_printk, "p7");
	dtor_pop(p5);
	dtor_pop(p1);
}
