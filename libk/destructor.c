#include "destructor.h"

#if INTERFACE

#include <stdint.h>

struct dtor_t {
	dtor_t * next;
	void (*dtor)(uintptr_t d);
	uintptr_t d;
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

dtor_t * dtor_push_frame(dtor_t * frame, void (*dtor)(uintptr_t d), uintptr_t d)
{
	frame->next = tls_get(dtor_get_key());

	/* Check the frame isn't already in use */
	if (frame->dtor) {
		kernel_panic("Destructor frame already in use: 0x%p/0x%p\n", frame->dtor, (void*)frame->d);
	}

	frame->dtor = dtor;
	frame->d = d;
	tls_set(dtor_get_key(), frame);

	return frame->next;
}

dtor_t * dtor_poll_frame()
{
	return tls_get(dtor_get_key());
}

void dtor_pop_frame(dtor_t * next)
{
	dtor_t * frame = tls_get(dtor_get_key());

	while(frame && next != frame) {
		if (frame->dtor) {
			frame->dtor(frame->d);
			frame->dtor = 0;
		}
		frame = frame->next;
	}
	tls_set(dtor_get_key(), frame);
}

void dtor_remove( void (*dtor)(uintptr_t d), uintptr_t d )
{
	dtor_t * frame = tls_get(dtor_get_key());

	while(frame) {
		if (dtor == frame->dtor && d == frame->d) {
			frame->dtor = 0;
			return;
		}
		frame = frame->next;
	}
}
