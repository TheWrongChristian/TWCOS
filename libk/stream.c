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
	va_list ap;
	uint32_t flags;
	int8_t width;
	int8_t base;
	int8_t precision;
	int8_t size;
	
};

#define FMT_FLAG_LEFT (1<<0)
#define FMT_FLAG_PLUS (1<<1)
#define FMT_FLAG_SPACE (1<<2)
#define FMT_FLAG_HASH (1<<3)
#define FMT_FLAG_ZERO (1<<4)
#define FMT_FLAG_UINT (1<<5)
#define FMT_FLAG_INT (1<<6)
#define FMT_FLAG_FLOAT (1<<7)
#define FMT_FLAG_PERCENT (1<<8)
#define FMT_FLAG_CHAR (1<<9)
#define FMT_FLAG_STR (1<<10)
#define FMT_FLAG_WRITTEN (1<<11)
#define FMT_FLAG_L (1<<12)
#define FMT_FLAG_LL (1<<13)
#define FMT_FLAG_H (1<<14)
#define FMT_FLAG_HH (1<<15)
#define FMT_FLAG_Z (1<<16)

static void stream_putuint64(stream_t * stream, struct fmt_opts * opts, uint64_t i)
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
		*pc = digits[i % opts->base];
		i /= opts->base;
	} while(i);
	stream_putstr(stream, pc);
}

static void stream_putint64(stream_t * stream, struct fmt_opts * opts, int64_t i)
{
	if (i<0) {
		stream_putc(stream, '-');
		i = -i;
		opts->flags &= ~(FMT_FLAG_PLUS | FMT_FLAG_SPACE);
	}
	stream_putuint64(stream, opts, i);
}

#if 0
static void stream_putuint(stream_t * stream, struct fmt_opts * opts)
{
	uint64_t val;
	if (opts->flags & FMT_FLAG_L) {
		val = (uint64_t)(unsigned long)va_arg(opts->ap, unsigned long);
	} else if (opts->flags & FMT_FLAG_LL) {
		val = (uint64_t)(unsigned long)va_arg(opts->ap, unsigned long long);
	} else if (opts->flags & FMT_FLAG_H) {
		val = (uint64_t)(unsigned short)va_arg(opts->ap, unsigned int);
	} else if (opts->flags & FMT_FLAG_HH) {
		val = (uint64_t)(unsigned char)va_arg(opts->ap, unsigned int);
	} else {
		val = (uint64_t)(unsigned int)va_arg(opts->ap, unsigned int);
	}
	stream_putuint64(stream, opts, val);
}

static void stream_putint(stream_t * stream, struct fmt_opts * opts)
{
	int64_t val;
	if (opts->flags & FMT_FLAG_L) {
		val = (int64_t)(long)va_arg(opts->ap, long);
	} else if (opts->flags & FMT_FLAG_LL) {
		val = (int64_t)(long long)va_arg(opts->ap, long long);
	} else if (opts->flags & FMT_FLAG_H) {
		val = (int64_t)(short)va_arg(opts->ap, int);
	} else if (opts->flags & FMT_FLAG_HH) {
		val = (int64_t)(signed char)va_arg(opts->ap, int);
	} else {
		val = (int64_t)(int)va_arg(opts->ap, int);
	}
	stream_putint64(stream, opts, val);
}
#else
static void stream_putuint(stream_t * stream, struct fmt_opts * opts)
{
	uint64_t val;
	if (opts->flags & FMT_FLAG_L) {
		val = va_arg(opts->ap, unsigned long);
	} else if (opts->flags & FMT_FLAG_LL) {
		val = va_arg(opts->ap, unsigned long long);
	} else if (opts->flags & FMT_FLAG_H) {
		val = (unsigned short)va_arg(opts->ap, unsigned int);
	} else if (opts->flags & FMT_FLAG_HH) {
		val = (unsigned char)va_arg(opts->ap, unsigned int);
	} else {
		val = va_arg(opts->ap, unsigned int);
	}
	stream_putuint64(stream, opts, val);
}

static void stream_putint(stream_t * stream, struct fmt_opts * opts)
{
	int64_t val;
	if (opts->flags & FMT_FLAG_L) {
		val = va_arg(opts->ap, long);
	} else if (opts->flags & FMT_FLAG_LL) {
		val = va_arg(opts->ap, long long);
	} else if (opts->flags & FMT_FLAG_H) {
		val = (short)va_arg(opts->ap, int);
	} else if (opts->flags & FMT_FLAG_HH) {
		val = (signed char)va_arg(opts->ap, int);
	} else {
		val = va_arg(opts->ap, int);
	}
	stream_putint64(stream, opts, val);
}
#endif
void stream_putstr(stream_t * stream, const char * s)
{
	char c;
	while((c = *s++)) {
		stream_putc(stream, c);
	}
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
		opts->width = va_arg(opts->ap, int);
	} else {
		opts->width = 0;
	}

	/* Precision */
	if ('.' == *s) {
		s++;
		if (isdigit(*s)) {
			s = parse_fmt_int(s, &opts->precision);
		} else if ('*' == *s) {
			opts->precision = va_arg(opts->ap, int);
			s++;
		}
	}

	/* Length */	
	if ('l' == *s) {
		s++;
		if ('l' == *s) {
			s++;
			opts->flags |= FMT_FLAG_LL;
		} else {
			opts->flags |= FMT_FLAG_L;
		}
	} else if ('h' == *s) {
		s++;
		if ('h' == *s) {
			s++;
			opts->flags |= FMT_FLAG_HH;
		} else {
			opts->flags |= FMT_FLAG_H;
		}
	} else if ('z' == *s) {
		s++;
		opts->flags |= FMT_FLAG_Z;
	}

	switch(*s++) {
	case '%':
		opts->flags |= FMT_FLAG_PERCENT;
		break;
	case 'c':
		opts->flags |= FMT_FLAG_CHAR;
		break;
	case 'd':
	case 'i':
		opts->flags |= FMT_FLAG_INT;
		opts->base = 10;
		break;
	case 'o':
		opts->flags |= FMT_FLAG_UINT;
		opts->base = 8;
		break;
	case 'x':
		opts->flags |= FMT_FLAG_UINT;
		opts->base = 16;
		break;
	case 'p':
		opts->flags |= FMT_FLAG_UINT | FMT_FLAG_HASH;
		opts->base = 16;
		break;
	case 's':
		opts->flags |= FMT_FLAG_STR;
		break;
	case 'n':
		opts->flags |= FMT_FLAG_WRITTEN;
		break;
	}

	return s;
}

static void * stream_getptr(struct fmt_opts * opts)
{
	return va_arg(opts->ap, void *);
}

int stream_vprintf(stream_t * stream, const char * fmt, va_list ap)
{
	long start = stream_tell(stream);
	char c;
	struct fmt_opts opts[1] = {0};

	va_copy(opts->ap, ap);
	while((c=*fmt++)) {
		if ('%' == c) {
			fmt = parse_fmt_opts(opts, fmt);
			if (opts->flags & FMT_FLAG_INT) {
				stream_putint(stream, opts);
			} else if (opts->flags & FMT_FLAG_UINT) {
				stream_putuint(stream, opts);
			} else if (opts->flags & FMT_FLAG_STR) {
				char * s = stream_getptr(opts);
				stream_putstr(stream, s);
			} else if (opts->flags & FMT_FLAG_CHAR) {
				char c = va_arg(opts->ap, char);
				stream_putc(stream, c);
			} else if (opts->flags & FMT_FLAG_PERCENT) {
				stream_putc(stream, '%');
			} else if (opts->flags & FMT_FLAG_WRITTEN) {
				int * p = stream_getptr(opts);
				*p = stream_tell(stream) - start;
			}
			opts->flags = opts->size = 0;
		} else {
			stream_putc(stream, c);
		}
	}
	va_end(opts->ap);

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

static char * vprintf_test(char * fmt, ...)
{
	static char buf[128];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, countof(buf), fmt, ap);
	va_end(ap);

	return buf;
}

char * printf_test()
{
	const uint8_t a=1;
	const uint8_t b=10;
	const uint8_t c=3;
	const uint8_t d=4;
	return vprintf_test("%hhx:progif:%hhx:%hhx:%hhx", a, b, c, d);
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

