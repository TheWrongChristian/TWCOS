#include <stdint.h>
#include <stddef.h>

#include "exception.h"

#if INTERFACE
#include <setjmp.h>
#include <stdarg.h>

struct exception_def {
	const char * name;
	struct exception_def * parent;
};

struct exception_cause {
	struct exception_def * type;
	
	/* Exception location */
	char * file;
	int line;

	/* Exception message */
	char message[256];
};

struct exception_frame {
	jmp_buf env;

	/* Try block location */
	char * file;
	int line;

	int state;
	int caught;

	/* Exception cause if thrown */
	struct exception_cause * cause;
};

/*
 * Top level exception from which all others are derived
 */

#define EXCEPTION_DEF(type,parent) static struct exception_def exception_def_ ## type = { #type, &exception_def_ ## parent }

#define KTRY \
	setjmp(exception_push(__FILE__, __LINE__)->env); \
	while(!exception_finished(__FILE__, __LINE__)) \
		if (exception_try())
#define KCATCH(type) \
		else if (exception_match(&(exception_def_ ## type)))
#define KFINALLY \
		else if (exception_finally())

#define KTHROW(type,message) exception_throw(&exception_def_ ## type, __FILE__, __LINE__, message)
#define KTHROWF(type,message, ...) exception_throw(&exception_def_ ## type, __FILE__, __LINE__, message, __VA_ARGS__ )

EXCEPTION_DEF(TestException, Exception);

#endif
struct exception_def exception_def_Throwable = { "Throwable", 0 };
struct exception_def exception_def_Exception = { "Exception", &exception_def_Throwable };
struct exception_def exception_def_Error = { "Error", &exception_def_Throwable };

#define EXCEPTION_FRAMES 8
struct exception_stack {
	int level;
	exception_frame frames[EXCEPTION_FRAMES];
};

static tls_key exception_key;
static slab_type_t frames;
static slab_type_t causes;

enum estates { EXCEPTION_NEW = 0, EXCEPTION_TRYING, EXCEPTION_CATCHING, EXCEPTION_FINISHING };

exception_frame * exception_push(char * file, int line)
{
	if (0 == exception_key) {
		exception_key = tls_get_key();
		slab_type_create(&frames,sizeof(struct exception_stack));
		slab_type_create(&causes,sizeof(struct exception_cause));
	}

	struct exception_stack * stack = tls_get(exception_key);
	if (0 == stack) {
		stack = slab_alloc(&frames);
		stack->level = -1;
		tls_set(exception_key, stack);
	}

	++stack->level;
	if (stack->level == EXCEPTION_FRAMES) {
		/* FIXME: Nested exceptions too deep */
		kernel_panic("Nested exceptions too deep\n");
		return 0;
	} else {
		exception_frame * frame = stack->frames+stack->level;
		frame->line = line;
		frame->file = file;
		frame->cause = 0;
		frame->state = EXCEPTION_NEW;
		frame->caught = 0;

		return frame;
	}
}

void exception_throw(struct exception_def * type, char * file, int line, char * message, ...)
{
	struct exception_stack * stack = tls_get(exception_key);

	if (stack->level<0) {
		kernel_panic("Unhandled exception: %s:%s:%d\n", type->name, file, line);
	} else {
		va_list ap;
		va_start(ap,message);

		exception_frame * frame = stack->frames+stack->level;
		frame->cause = slab_alloc(&causes);
		frame->cause->type = type;
		frame->cause->file = file;
		frame->cause->line = line;
		vsnprintf(frame->cause->message, sizeof(frame->cause->message), message, ap);
		va_end(ap);

		longjmp(frame->env, 1);
	}
}

int exception_finished(char * file, int line)
{
	struct exception_stack * stack = tls_get(exception_key);
	exception_frame * frame = stack->frames+stack->level;

	while(frame->file != file || frame->line != line) {
		/* FIXME: Report the error */
		stack->level--;
		if (stack->level<0) {
			kernel_panic("Exception stack empty!\n");
		}
		frame = stack->frames+stack->level;
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
		stack->level--;
		if (frame->cause && 0 == frame->caught) {
			exception_throw(frame->cause->type, frame->cause->file, frame->cause->line, frame->cause->message);
		}
		return 1;
	}

	return 0;
}

int exception_try()
{
	struct exception_stack * stack = tls_get(exception_key);
	exception_frame * frame = stack->frames+stack->level;

	if (EXCEPTION_TRYING == frame->state) {
		return 1;
	}

	return 0;
}

int exception_match( struct exception_def * match )
{
	struct exception_stack * stack = tls_get(exception_key);
	exception_frame * frame = stack->frames+stack->level;

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

int exception_finally()
{
	struct exception_stack * stack = tls_get(exception_key);
	exception_frame * frame = stack->frames+stack->level;

	return (EXCEPTION_FINISHING == frame->state);
}

static void do_throw()
{
	KTHROW(TestException, "An exception");
}

void exception_test()
{
	KTRY {
		do_throw();
	} KCATCH(Exception) {
		kernel_printk("Caught exception\n");
	}
}
