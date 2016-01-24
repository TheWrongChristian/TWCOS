#include <stdarg.h>
#include <stdint.h>

#include "stream.h"

static void stream_putc(struct stream * stream, char c)
{
	stream->ops->putc(stream, c);
}

static int stream_tell(struct stream * stream)
{
	return stream->ops->tell(stream);
}

#ifndef abs
#define abs(i) ((i<0) ? -i : i)
#endif

static char * digits = "0123456789abcdef";
static void stream_putuint(struct stream * stream, int base, uint32_t i)
{
	if (abs(i)<base) {
		stream_putc(stream, digits[i]);
	} else {
		stream_putuint(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

static void stream_putulong(struct stream * stream, int base, uint64_t i)
{
	if (i<base) {
		stream_putc(stream, digits[i]);
	} else {
		stream_putulong(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

static void stream_putint(struct stream * stream, int base, int i)
{
	if (i<base) {
		if (i<0) {
			stream_putc(stream, '-');
		}
		stream_putc(stream, digits[i]);
	} else {
		stream_putint(stream, base, i / base);
		stream_putc(stream, digits[i%base]);
	}
}

static void stream_putptr(struct stream * stream, const void * p)
{
	stream_putulong(stream, 16, (unsigned int)p);
}

static void stream_putstr(struct stream * stream, const char * s)
{
	char c;
	while((c = *s++)) {
		stream_putc(stream, c);
	}
}

struct fmt_opts {
	int rjustify;
	int pad0;
	int longint;
	int alternate;
};

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

int stream_vprintf(struct stream * stream, const char * fmt, va_list ap)
{
	long start = stream_tell(stream);
	char c;

	while(c=*fmt++) {
		if ('%' == c) {
			switch(c = *fmt++) {
			case '%':
				stream_putc(stream, '%');
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
int stream_printf(struct stream * stream, const char * fmt, ...)
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
struct stream_null {
	struct stream stream;
	long chars;
} snull;

static void null_putc(struct stream * stream, char c)
{
	struct stream_null * snull = (struct stream_null *)stream;

	snull->chars++;
}

static long null_tell(struct stream * stream)
{
	struct stream_null * snull = (struct stream_null *)stream;
	return snull->chars;
}

static struct stream_ops null_ops = {
	putc: null_putc,
	tell: null_tell
};

struct stream * null_stream()
{
	snull.stream.ops = &null_ops;
	snull.chars = 0;

	return (struct stream *)&snull;
}

#if INTERFACE
#include <stdarg.h>

struct stream {
	struct stream_ops * ops;
};

struct stream_ops {
	void (*putc)(struct stream * stream, char c);
	long (*tell)(struct stream * stream);
};
#endif

