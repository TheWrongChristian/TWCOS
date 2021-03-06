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

int memcmp(const void *s1, const void *s2, size_t n)
{
	const char * c1 = s1;
	const char * c2 = s2;
	for(int i=0; i<n; i++) {
		if (*c1<*c2) {
			return -1;
		} else if (*c1>*c2) {
			return 1;
		}
	}

	return 0;
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

int strlen(const char * s)
{
	int l = 0;
	while(s[l]) {
		l++;
	}

	return l;
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

const char * strchr(const char * s, int c)
{
	for(int i=0; s[i]; i++) {
		if (c == s[i]) {
			return s+i;
		}
	}

	return 0;
}

const char * strrchr(const char * s, int c)
{
	const char * last = 0;

	/* FIXME: UTF-8 */
	for(int i=0; s[i]; i++) {
		if (c == s[i]) {
			last = s+i;
		}
	}

	return last;
}

int starts_with(const char * s, const char * head)
{
	int slen = strlen(s);
	int hlen = strlen(head);

	if (hlen > slen) {
		return 0;
	}

	return (0 == strncmp(s, head, hlen));
}

int ends_with(const char * s, const char * tail)
{
	int slen = strlen(s);
	int tlen = strlen(tail);

	if (tlen > slen) {
		return 0;
	}

	return (0 == strcmp(s+slen-tlen, tail));
}

/*
 * Common constants for dirname and basename
 */
static char dot[] = ".";
static char slash[] = "/";

static char * basedirname(char * path, int dirname)
{
	/*
	 * We follow POSIX behaviour, which may change path
	 * and return a pointer within path.
	 */
	int plen = strlen(path);

	while(plen && '/' == path[plen-1]) {
		/* Strip trailing / */
		path[plen-1] = 0;
		plen--;
	}

	char * lastslash = (char*)strrchr(path, '/');

	if (0 == lastslash) {
		/* No slash */
		if (dirname) {
			/* dirname is . */
			return dot;
		} else {
			/* basename is path */
			return path;
		}
	} else if (lastslash == path) {
		/* Only slash is root */
		if (dirname) {
			return slash;
		} else {
			return path+1;
		}
	} else {
		*lastslash = 0;
		if (dirname) {
			return path;
		} else {
			return lastslash+1;
		}
	}
}

char * dirname(char * path)
{
	return basedirname(path, 1);
}
char * basename(char * path)
{
	return basedirname(path, 0);
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
