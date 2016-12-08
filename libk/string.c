#include <stddef.h>
#include "string.h"

void * memset(void *s, int c, size_t n)
{
	char * cp = s;
	int i;

	for(i=0; i<n; i++, cp++) {
		*cp = c;
	}

	return s;
}

int strcmp( const char * s1, const char * s2 )
{
	while(*s1 && *s2 && *s1 == *s2) {
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

struct stream_string {
        struct stream stream;
	char * buf;
	int size;
        long chars;
};

static void string_putc(struct stream * stream, char c)
{
        struct stream_string * sstring = (struct stream_console *)stream;
	if (sstring->chars < sstring->size) {
		sstring->buf[sstring->chars] = c;
	}
	sstring->chars++;
}

static long string_tell(struct stream * stream)
{
        struct stream_string * sstring = (struct stream_console *)stream;
	return sstring->chars;
}

static struct stream_ops string_ops = {
        putc: string_putc,
        tell: string_tell
};

int snprintf(char * buf, int size, char * fmt, ...)
{
	int chars;
	va_list ap;
	va_start(ap,fmt);
	chars = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	return chars;
}

int vsnprintf(char * buf, int size, char * fmt, va_list ap)
{
	struct stream_string sstring[1] = {
		{ buf : buf, size: size, chars: 0 }
	};
	sstring->stream.ops = &string_ops;
	if (stream_vprintf(sstring, fmt, ap)<size) {
		sstring->buf[sstring->chars] = 0x0;
	}

	return sstring->chars;
}
