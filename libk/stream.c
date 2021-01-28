#include <stdarg.h>
#include <stdint.h>

#include "stream.h"

static void stream_putc(stream_t * stream, char c)
{
	stream->ops->putc(stream, c);
}

static int stream_tell(stream_t * stream)
{
	return stream->ops->tell(stream);
}

#ifndef abs
#define abs(i) ((i<0) ? -i : i)
#endif

static char * digits = "0123456789abcdef";
static void stream_putuint(stream_t * stream, int base, uint32_t i)
{
	if (i<base) {
		stream_putc(stream, digits[i]);
	} else {
		stream_putuint(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

static void stream_putulong(stream_t * stream, int base, uint64_t i)
{
	if (i<base) {
		stream_putc(stream, digits[i]);
	} else {
		stream_putulong(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

static void stream_putint(stream_t * stream, int base, int i)
{
	if (i<0) {
		stream_putc(stream, '-');
		i = -i;
	}
	if (i<base) {
		stream_putc(stream, digits[i]);
	} else {
		stream_putint(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

void stream_putstr(stream_t * stream, const char * s)
{
	char c;
	while((c = *s++)) {
		stream_putc(stream, c);
	}
}

static void stream_putptr(stream_t * stream, const void * p)
{
	stream_putstr(stream, "0x");
	stream_putulong(stream, 16, (unsigned int)p);
}

struct fmt_opts {
	int rjustify;
	int pad0;
	int longint;
	int alternate;
};

#if 0
static void parse_fmt_opts( struct fmt_opts * opts, const char * s )
{
	opts->rjustify = opts->pad0 = opts->longint = opts->alternate = 0;
	switch(*s++) {
	case '#':
		opts->alternate = 1;
		break;
	case '0':
		while(*s >= '0' && *s <= '9') {
			opts->pad0 *= 10;
			opts->pad0 += (*s - '0');
			s++;
		};
		break;	
	case '-':
		break;	
	}
}
#endif

int stream_vprintf(stream_t * stream, const char * fmt, va_list ap)
{
	long start = stream_tell(stream);
	char c;

	while((c=*fmt++)) {
		if ('%' == c) {
			switch(c = *fmt++) {
			case '%':
				stream_putc(stream, '%');
				break;
			case 'c':
				stream_putc(stream, va_arg(ap, int));
				break;
			case 'd':
			case 'i':
				stream_putint(stream, 10, va_arg(ap, int));
				break;
			case 'o':
				stream_putuint(stream, 8, va_arg(ap, int));
				break;
			case 'x':
				stream_putuint(stream, 16, va_arg(ap, int));
				break;
			case 'p':
				stream_putptr(stream, va_arg(ap, const void *));
				break;
			case 's':
				stream_putstr(stream, va_arg(ap, const char *));
				break;
			case 'n':
				*(va_arg(ap, int *)) = stream_tell(stream) - start;
				break;
			default:
				stream_putc(stream, c);
				break;
			}
		} else {
			stream_putc(stream, c);
		}
	}

	va_end(ap);

	return stream_tell(stream) - start;
}
int stream_printf(stream_t * stream, const char * fmt, ...)
{
	int len = 0;
	va_list ap;
	va_start(ap, fmt);

	len = stream_vprintf(stream, fmt, ap);

	va_end(ap);

	return len;
}

/*
 * null stream - Discard any characters
 */
typedef struct stream_null stream_null_t;
struct stream_null {
	stream_t stream;
	long chars;
} snull;

static void null_putc(stream_t * stream, char c)
{
	stream_null_t * snull = container_of(stream, stream_null_t, stream);

	snull->chars++;
}

static long null_tell(stream_t * stream)
{
	stream_null_t * snull = container_of(stream, stream_null_t, stream);
	return snull->chars;
}

static stream_ops_t null_ops = {
	putc: null_putc,
	tell: null_tell
};

stream_t * null_stream()
{
	snull.stream.ops = &null_ops;
	snull.chars = 0;

	return &snull.stream;
}

typedef struct stream_vnode_t {
	stream_t stream;
	vnode_t * vnode;
	off64_t offset;
} stream_vnode_t;

static void vnode_putc(stream_t * stream, char c)
{
	stream_vnode_t * svnode = container_of(stream, stream_vnode_t, stream);
	vnode_write(svnode->vnode, svnode->offset, &c, 1);
	svnode->offset++;
}

static long vnode_tell(stream_t * stream)
{
	stream_vnode_t * svnode = container_of(stream, stream_vnode_t, stream);
	return svnode->offset;
}

static stream_ops_t stream_vnode_ops = {
	putc: vnode_putc,
	tell: vnode_tell
};

stream_t * vnode_stream(vnode_t * vnode)
{
	stream_vnode_t * stream = calloc(1, sizeof(*stream));
	stream->stream.ops = &stream_vnode_ops;
	stream->vnode = vnode;

	return stream;
}

#if INTERFACE
#include <stdarg.h>

typedef struct stream_t stream_t;
typedef struct stream_ops_t stream_ops_t;

struct stream_t {
	stream_ops_t * ops;
};

struct stream_ops_t {
	void (*putc)(stream_t * stream, char c);
	long (*tell)(stream_t * stream);
};

#endif

