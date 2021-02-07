#include "destructor.h"

#if INTERFACE

#include <stdint.h>

typedef void (*dtor_f)(void * p);

struct dtor_t {
	int exception;
	dtor_t * next;
	dtor_f dtor;
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
static spin_t frames_lock;
static GCROOT dtor_t ** frames = 0;

void dtor_pop_common(dtor_t * until, int exception)
{
	dtor_t * frame = tls_get(dtor_get_key());
	dtor_t * first = frame;
	dtor_t * last = frame;

	if (frame == until) {
		/* Nothing to do */
		return;
	}

	while(frame && until != frame) {
		if (frame->dtor && frame->exception >= exception) {
			frame->dtor(frame->p);
			frame->dtor = 0;
			frame->p = 0;
		}
		last = frame;
		frame = frame->next;
	}

#if 0
	/*
	 * By here, frame first will be the head of the
	 * frames we're freeing, and last is the last frame
	 * we're freeing.
	 */
	SPIN_AUTOLOCK(&frames_lock) {
		last->next = frames[0];
		frames[0] = first;
	}
#endif

	tls_set(dtor_get_key(), frame);
}

void dtor_pop_exception(dtor_t * until)
{
	dtor_pop_common(until, 1);
}

void dtor_pop(dtor_t * until)
{
	dtor_pop_common(until, 0);
}

static dtor_t * dtor_push_common(void (*dtor)(void * p), void * p, int exception)
{
	dtor_t * frame = 0;
	frames = calloc(1, sizeof(*frames));
#if 0
	SPIN_AUTOLOCK(&frames_lock) {
		if (0 == frames) {
			frames = calloc(1, sizeof(*frames));
		}
		if (frames[0]) {
			frame = frames[0];
			frames[0] = frame->next;
		} else {
			frame = calloc(1, sizeof(*frame));
		}
	}
#endif

	frame->exception = exception;
	frame->next = tls_get(dtor_get_key());
	frame->dtor = dtor;
	frame->p = p;

	tls_set(dtor_get_key(), frame);

	return frame->next;
}

dtor_t * dtor_push_exception(void (*dtor)(void * p), void * p)
{
	return dtor_push_common(dtor, p, 1);
}

dtor_t * dtor_push(void (*dtor)(void * p), void * p)
{
	return dtor_push_common(dtor, p, 0);
}

void * dtor_remove( void (*dtor)(void * p), void * p )
{
	dtor_t * frame = tls_get(dtor_get_key());

	while(frame) {
		if (dtor == frame->dtor && p == frame->p) {
			frame->dtor = 0;
			return p;
		}
		frame = frame->next;
	}

	return p;
}

void dtor_test()
{
	dtor_t * p1 = dtor_push((dtor_f)kernel_printk, "p1");
	dtor_push((dtor_f)kernel_printk, "p2");
	dtor_push((dtor_f)kernel_printk, "p3");
	dtor_push((dtor_f)kernel_printk, "p4");
	dtor_t * p5 = dtor_push((dtor_f)kernel_printk, "p5");
	dtor_push((dtor_f)kernel_printk, "p6");
	dtor_push((dtor_f)kernel_printk, "p7");
	dtor_pop(p5);
	dtor_pop(p1);
}
