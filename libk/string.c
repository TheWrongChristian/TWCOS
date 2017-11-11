#include "string.h"

#if INTERFACE
#include <stddef.h>
#include <stdarg.h>
#endif

void * memset(void *s, int c, size_t n)
{
	char * cp = s;
	int i;

	for(i=0; i<n; i++, cp++) {
		*cp = c;
	}

	return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const char * cs = src;
	char * cd = dest;

	for(int i=0; i<n; i++) {
		cd[i] = cs[i];
	}

	return dest;
}

int strcmp( const char * s1, const char * s2 )
{
	while(*s1 && *s2 && *s1 == *s2) {
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

int strncmp( const char * s1, const char * s2, size_t n )
{
	for(int i=0; i<n && *s1 && *s2 && *s1 == *s2; i++) {
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

#if 0
char * strdup( const char * s)
{
	int len = strlen(s);
	char * ret = malloc(len + 1);

	ret[len] = 0;
	return memcpy(ret, s, len);
}
#endif

char * strndup( const char * s, int len)
{
	char * ret = malloc(len + 1);

	ret[len] = 0;
	return memcpy(ret, s, len);
}

char ** ssplit( const char * str, int sep )
{
	int i = 0;
	int max = 8;
	char ** strs = tmalloc(max * sizeof(*strs));
	const char * s = str;
	strs[0] = 0;

	while(1) {
		const char * start = s;
		while(*s && *s != sep) {
			s++;
		}

		/* Copy the (copied) string to it's destination */
		int len = s - start;
		if (len > 0) {
			strs[i] = strndup(start, len);
		} else {
			strs[i] = "";
		}

		/* Expand the array if necessary */
		if (++i == max) {
			/* Expand strs */
			char ** oldstrs = strs;
			max += 8;
			strs = tmalloc(max * sizeof(*strs));
			for(int n = 0; n<i; n++) {
				strs[n] = oldstrs[n];
			}
		}

		if (!*s) {
			/* End of input */
			char ** ret = malloc(sizeof(*ret) * i+1);
			ret[i] = 0;
			for(int n = 0; n<i; n++) {
				ret[n] = strs[n];
			}
			return ret;
		} else {
			s++;
		}
	}
	/* Not reached */
	return 0;
}

typedef struct stream_string stream_string_t;
struct stream_string {
        stream_t stream;
	char * buf;
	int size;
        long chars;
};

static void string_putc(stream_t * stream, char c)
{
        stream_string_t * sstring = container_of(stream, stream_string_t, stream);
	if (sstring->chars < sstring->size) {
		sstring->buf[sstring->chars] = c;
	}
	sstring->chars++;
}

static long string_tell(stream_t * stream)
{
        stream_string_t * sstring = container_of(stream, stream_string_t, stream);
	return sstring->chars;
}

static stream_ops_t string_ops = {
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
	stream_string_t sstring[1] = {
		{ buf : buf, size: size, chars: 0 }
	};
	sstring->stream.ops = &string_ops;
	if (stream_vprintf(&sstring->stream, fmt, ap)<size) {
		sstring->buf[sstring->chars] = 0x0;
	}

	return sstring->chars;
}
