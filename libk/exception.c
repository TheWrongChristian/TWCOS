#include <stdint.h>
#include <stddef.h>

#include "exception.h"

#if INTERFACE
#include <setjmp.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct exception_def {
        const char * name;
        struct exception_def * parent;
} exception_def;

struct exception_cause {
	exception_def * type;
	
	/* Exception location */
	char * file;
	int line;

	/* Exception message */
	char message[256];
};

struct exception_frame {
	/* Try block location */
	char * file;
	int line;

	int state;
	int caught;

	/* State */
	jmp_buf env;
	dtor_t * dtor_frame;

	/* Exception chain */
	struct exception_frame * next;

	/* Exception cause if thrown */
	struct exception_cause * cause;
};

/*
 * Top level exception from which all others are derived
 */

#define EXCEPTION_FRAME2(x, y) x ## y
#define EXCEPTION_FRAME(line) EXCEPTION_FRAME2(f,line)
#define KTRY \
	exception_frame EXCEPTION_FRAME(__LINE__) = { __FILE__, __LINE__ }; \
	setjmp(exception_push(&EXCEPTION_FRAME(__LINE__))->env); \
	while(!exception_finished(__FILE__, __LINE__)) \
		if (exception_try())
#define KCATCH(type) \
		else if (exception_match(&type))
#define KFINALLY \
		else if (exception_finally())

#define KTHROW(type,message) exception_throw(&type, __FILE__, __LINE__, message)
#define KTHROWF(type,message, ...) exception_throw(&type, __FILE__, __LINE__, message, __VA_ARGS__ )
#define KRETHROW() exception_rethrow()

#define EXCEPTION_DEF(type,parent) static exception_def type = { #type, &parent }
EXCEPTION_DEF(TestException, Exception);

#endif
exception_def Throwable = { "Throwable", 0 };
exception_def Exception = { "Exception", &Throwable };
exception_def Error = { "Error", &Throwable };
exception_def RuntimeException = { "RuntimeException", &Exception };

static tls_key exception_key;
static slab_type_t causes[1] = {SLAB_TYPE(sizeof(struct exception_cause), 0, 0)};

enum estates { EXCEPTION_NEW = 0, EXCEPTION_TRYING, EXCEPTION_CATCHING, EXCEPTION_FINISHING };

exception_frame * exception_push(exception_frame * frame)
{
	if (0 == exception_key) {
		exception_key = tls_get_key();
	}

	/* Link the frame into the chain */
	frame->next = tls_get(exception_key);
	tls_set(exception_key, frame);

	frame->cause = 0;
	frame->state = EXCEPTION_NEW;
	frame->caught = 0;
	frame->dtor_frame = dtor_poll_frame();

	return frame;
}

static void exception_throw_cause(struct exception_cause * cause)
{
	struct exception_frame * frame = tls_get(exception_key);

	frame->cause = cause;

	longjmp(frame->env, 1);
}

void exception_throw(exception_def * type, char * file, int line, char * message, ...)
{
	va_list ap;
	va_start(ap,message);

	struct exception_cause * cause = slab_calloc(causes);
	cause->type = type;
	cause->file = file;
	cause->line = line;
	vsnprintf(cause->message, sizeof(cause->message), message, ap);
	va_end(ap);

	exception_throw_cause(cause);
}

void exception_rethrow()
{
	/* Propagate the exception */
	exception_frame * frame = tls_get(exception_key);
	frame->caught = 0;
}

int exception_finished(char * file, int line)
{
	exception_frame * frame = tls_get(exception_key);

	while(frame->file != file || frame->line != line) {
		/* FIXME: Report the error */
		frame = frame->next;
		if (0 == frame) {
			kernel_panic("Exception stack empty!\n");
		}
	}

	switch(frame->state) {
	case EXCEPTION_NEW:
		frame->state = EXCEPTION_TRYING;
		return 0;
	case EXCEPTION_TRYING:
		if (frame->cause) {
			frame->state = EXCEPTION_CATCHING;
                        return 0;
		}
		/* Fall through */
	case EXCEPTION_CATCHING:
		frame->state = EXCEPTION_FINISHING;
		return 0;
	case EXCEPTION_FINISHING:
		dtor_pop(frame->dtor_frame);
		tls_set(exception_key, frame->next);
		if (frame->cause && 0 == frame->caught) {
			exception_throw_cause(frame->cause);
		}
		return 1;
	}

	return 0;
}

int exception_try()
{
	exception_frame * frame = tls_get(exception_key);

	if (EXCEPTION_TRYING == frame->state) {
		return 1;
	}

	return 0;
}

int exception_match( exception_def * match )
{
	exception_frame * frame = tls_get(exception_key);

	if (EXCEPTION_CATCHING == frame->state) {
		exception_cause * cause = frame->cause;
		exception_def * type = (cause) ? cause->type : 0;
		while(type) {
			if (0 == strcmp(type->name, match->name)) {
				frame->caught = 1;
				return 1;
			}
			type = type->parent;
		}
	}

	return 0;
}

char * exception_message()
{
	exception_frame * frame = tls_get(exception_key);

	if (frame->cause) {
		return frame->cause->message;
	}

	return "No exception";
}

int exception_finally()
{
	exception_frame * frame = tls_get(exception_key);

	return (EXCEPTION_FINISHING == frame->state);
}

static void do_throw()
{
	KTRY {
		KTHROW(TestException, "An exception");
	} KFINALLY {
		kernel_printk("KTRY/KFINALLY\n");
	}
}

void exception_test()
{
	KTRY {
		do_throw();
	} KCATCH(Exception) {
		kernel_printk("Caught exception\n");
	}
}
