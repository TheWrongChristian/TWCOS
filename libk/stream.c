#include <stdarg.h>
#include <stdint.h>

#include "stream.h"

static void stream_putc(stream_t * stream, char c)
{
	stream->ops->putc(stream, c);
}

static long stream_tell(stream_t * stream)
{
	return stream->ops->tell(stream);
}

#ifndef abs
#define abs(i) ((i<0) ? -i : i)
#endif

static char * digits = "0123456789abcdef";

struct fmt_opts {
	uint8_t flags;
	int8_t width;
	int8_t precision;
	int8_t length;
};

#define FMT_FLAG_LEFT (1<<0)
#define FMT_FLAG_PLUS (1<<1)
#define FMT_FLAG_SPACE (1<<2)
#define FMT_FLAG_HASH (1<<3)
#define FMT_FLAG_ZERO (1<<4)

static void stream_putuint64(stream_t * stream, struct fmt_opts * opts, int base, uint64_t i)
{
	if (opts->flags & FMT_FLAG_PLUS) {
		stream_putc(stream, '+');
	} else if (opts->flags & FMT_FLAG_SPACE) {
		stream_putc(stream, ' ');
	}
	char buf[32];
	char * pc = buf + countof(buf)-1;
	*pc = 0;
	do {
		pc--;
		*pc = digits[i % base];
		i /= base;
	} while(i);
	stream_putstr(stream, pc);
}

static void stream_putint64(stream_t * stream, struct fmt_opts * opts, int base, int64_t i)
{
	if (i<0) {
		stream_putc(stream, '-');
		i = -i;
		opts->flags &= ~(FMT_FLAG_PLUS | FMT_FLAG_SPACE);
	}
	stream_putuint64(stream, opts, base, i);
}

static void stream_getwidthprecision(struct fmt_opts * opts, va_list * ap)
{
	if (opts->width<0) {
		/* Consume a width argument */
		opts->width = va_arg(*ap, int);
	}
	if (opts->precision<0) {
		/* Consume a precision argument */
		opts->precision = va_arg(*ap, int);
	}
}

static void stream_putuint(stream_t * stream, struct fmt_opts * opts, int base, va_list * ap)
{
	stream_getwidthprecision(opts, ap);

	uint64_t val;
#if 0
	switch(opts->length) {
	case sizeof(char):
		val = va_arg(*ap, unsigned int);
		break;
	case sizeof(uint16_t):
		val = va_arg(*ap, unsigned int);
		break;
	case sizeof(uint32_t):
		val = va_arg(*ap, uint32_t);
		break;
	case sizeof(uint64_t):
		val = va_arg(*ap, uint64_t);
		break;
	default:
		val = 0;
		break;
	}
#endif
	if (opts->length == sizeof(unsigned long)) {
		val = va_arg(*ap, unsigned long);
	} else if (opts->length == sizeof(unsigned long long)) {
		val = va_arg(*ap, unsigned long long);
	} else {
		val = va_arg(*ap, unsigned int);
	}
	stream_putuint64(stream, opts, base, val);
}

static void stream_putint(stream_t * stream, struct fmt_opts * opts, int base, va_list * ap)
{
	stream_getwidthprecision(opts, ap);

	int64_t val;
#if 0
	switch(opts->length) {
	case sizeof(char):
		val = va_arg(*ap, int);
		break;
	case sizeof(int16_t):
		val = va_arg(*ap, int);
		break;
	case sizeof(int32_t):
		val = va_arg(*ap, int32_t);
		break;
	case sizeof(int64_t):
		val = va_arg(*ap, int64_t);
		break;
	default:
		val = 0;
		break;
	}
#endif
	if (opts->length == sizeof(long)) {
		val = va_arg(*ap, long);
	} else if (opts->length == sizeof(long long)) {
		val = va_arg(*ap, long long);
	} else {
		val = va_arg(*ap, int);
	}
	stream_putint64(stream, opts, base, val);
}

void stream_putstr(stream_t * stream, const char * s)
{
	char c;
	while((c = *s++)) {
		stream_putc(stream, c);
	}
}

static void stream_putptr(stream_t * stream, void * p)
{
	struct fmt_opts opts[1] = {0};
	stream_putstr(stream, "0x");
	stream_putuint64(stream, opts, 16, (uintptr_t)p);
}

static const char * parse_fmt_int(const char * s, int8_t * pval)
{
	int8_t val=0;
	while(*s >= '0' && *s <= '9') {
		val *= 10;
		val += (*s - '0');
		s++;
	};
	*pval = val;
	return s;
}

static const char * parse_fmt_flags(struct fmt_opts * opts, const char * s )
{
	while(1) {
		switch(*s) {
		case '-':
			opts->flags |= FMT_FLAG_LEFT;
			break;
		case '+':
			opts->flags |= FMT_FLAG_PLUS;
			break;
		case ' ':
			opts->flags |= FMT_FLAG_SPACE;
			break;
		case '#':
			opts->flags |= FMT_FLAG_HASH;
			break;
		case '0':
			opts->flags |= FMT_FLAG_ZERO;
			break;
		default:
			return s;
		}
		s++;
	}
}

static const char * parse_fmt_opts( struct fmt_opts * opts, const char * s )
{
	s = parse_fmt_flags(opts, s);

	/* Width */
	if (isdigit(*s)) {
		s = parse_fmt_int(s, &opts->width);
	} else if ('*' == *s) {
		opts->width = -1;
	} else {
		opts->width = 0;
	}

	/* Precision */
	if ('.' == *s) {
		s++;
		if (isdigit(*s)) {
			s = parse_fmt_int(s, &opts->precision);
		} else if ('*' == *s) {
			opts->precision = -1;
			s++;
		}
	}

	/* Length */	
	opts->length = sizeof(int);
	if ('l' == *s) {
		s++;
		if ('l' == *s) {
			s++;
			opts->length = sizeof(long long);
		} else {
			opts->length = sizeof(long);
		}
	} else if ('h' == *s) {
		s++;
		if ('h' == *s) {
			s++;
			opts->length = sizeof(char);
		} else {
			opts->length = sizeof(short);
		}
	} else if ('z' == *s) {
		s++;
		opts->length = sizeof(size_t);
	}

	return s;
}

int stream_vprintf(stream_t * stream, const char * fmt, va_list ap)
{
	long start = stream_tell(stream);
	char c;

	while((c=*fmt++)) {
		if ('%' == c) {
			struct fmt_opts opts[1] = {0};
			fmt = parse_fmt_opts(opts, fmt);
			switch(c = *fmt++) {
			case '%':
				stream_putc(stream, '%');
				break;
			case 'c':
				stream_putc(stream, va_arg(ap, char));
				break;
			case 'd':
			case 'i':
				stream_putint(stream, opts, 10, &ap);
				break;
			case 'o':
				stream_putuint(stream, opts, 8, &ap);
				break;
			case 'x':
				stream_putuint(stream, opts, 16, &ap);
				break;
			case 'p':
				stream_putptr(stream, va_arg(ap, void *));
				break;
			case 's':
				stream_putstr(stream, va_arg(ap, char *));
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
	.putc = null_putc,
	.tell = null_tell
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
	.putc = vnode_putc,
	.tell = vnode_tell
};

stream_t * vnode_stream(vnode_t * vnode)
{
	stream_vnode_t * stream = calloc(1, sizeof(*stream));
	stream->stream.ops = &stream_vnode_ops;
	stream->vnode = vnode;

	return &stream->stream;
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

