#include "string.h"

#if INTERFACE
#include <stddef.h>
#include <stdarg.h>

struct cbuffer_t {
	char * buf;
	int capacity;
	int len;
};

#endif

void * memset(void *s, int c, size_t n)
{
	/*
	 * -cccIIIIIIIIIIIIcc--
	 *  ^  ^           ^ ^
	 *  |  |           | |
         *  |  |           | end
	 *  |  |           p4end
	 *  |  p1end/p4
	 *  p1
	 */
	char * p = s;
	char * p1end = (char*)PTR_ALIGN_NEXT(p, sizeof(uint32_t));
	uint32_t * p4 = (uint32_t*)p1end;

	char * end = p + n;
	uint32_t * p4end = (uint32_t*)PTR_ALIGN(end, sizeof(uint32_t));

	for(;p<p1end; p++) {
		*p = c;
	}

	c |= c<<8;
	c |= c<<16;
	for(;p4<p4end; p4++) {
		*p4 = c;
	}

	p = (char*)p4end;
	for(;p<end; p++) {
		*p = c;
	}

	return s;
}

void *memmove(void *dest, const void *src, size_t n)
{
	const char * cs = src;
	char * cd = dest;
	int overlap;

	if (cs<cd) {
		overlap=(cs+n>=cd);
	} else if (cs>cd) {
		overlap=0;
	} else {
		/* No move at all! */
		return cd;
	}

	if (overlap) {
		/* Slow byte copy version */
		for(int i=n-1; i>=0; i--) {
			cd[i] = cs[i];
		}
		return dest;
	} else {
		return memcpy(dest, src, n);
	}
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const char * cs = src;
	char * cd = dest;
	uint32_t * cs4 = (uint32_t*)PTR_ALIGN(cs, sizeof(uint32_t));
	uint32_t * cd4 = (uint32_t*)PTR_ALIGN(cd, sizeof(uint32_t));

	if ((uint32_t *)src == cs4 && (uint32_t *)dest == cd4 && 0==n%sizeof(uint32_t)) {
		uint32_t * cs4end = cs4 + n/sizeof(uint32_t);

		switch((n>>2)&03) {
			do {
			case 0:
				*cd4++ = *cs4++;
			case 3:
				*cd4++ = *cs4++;
			case 2:
				*cd4++ = *cs4++;
			case 1:
				*cd4++ = *cs4++;
			} while(cs4<cs4end);
		}
#if 0
		for(int i=0; i<n/sizeof(uint32_t); i++) {
			cd4[i] = cs4[i];
		}
#endif
	} else {
		for(int i=0; i<n; i++) {
			cd[i] = cs[i];
		}
	}

	return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const char * c1 = s1;
	const char * c2 = s2;
	for(int i=0; i<n; i++) {
		if (c1[i]<c2[i]) {
			return -1;
		} else if (c1[i]>c2[i]) {
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

char * strcpy(char * dest, const char * src)
{
	int i;
	for(i=0; src[i]; i++) {
		dest[i] = src[i];
	}
	dest[i] = 0;

	return dest;
}

char * strncpy(char * dest, const char * src, size_t maxlen)
{
	int i;
	for(i=0; src[i] && i<maxlen; i++) {
		dest[i] = src[i];
	}
	dest[i] = 0;

	return dest;
}

char * strdup( const char * s)
{
	int len = strlen(s);
	char * ret = malloc(len + 1);

	ret[len] = 0;
	return memcpy(ret, s, len);
}

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
	arena_state state = arena_getstate(NULL);
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
			arena_setstate(NULL, state);
			return ret;
		} else {
			s++;
		}
	}
	/* Not reached */
	arena_setstate(NULL, state);
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

int snprintf(char * buf, int size, const char * fmt, ...)
{
	int chars;
	va_list ap;
	va_start(ap,fmt);
	chars = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	return chars;
}

int vsnprintf(char * buf, int size, const char * fmt, va_list ap)
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

void cbuffer_init(cbuffer_t * cbuf, int capacity)
{
	cbuf->buf = malloc(capacity);
	cbuf->capacity = capacity;
}

static char * cbuffer_charat(cbuffer_t * cbuf, int index)
{
	if (index>=cbuf->len) {
		return 0;
	}

	if (index < 0) {
		index = cbuf->len + index;
		if (index < 0) {
			return 0;
		}
	}

	assert(cbuf->len>index);

	return cbuf->buf+index;
}

static void cbuffer_resize(cbuffer_t * cbuf, int capacity)
{
	cbuf->buf = realloc(cbuf->buf, capacity);
	cbuf->capacity = capacity;
}

void cbuffer_putat(cbuffer_t * cbuf, int index, char c)
{
	char * cp = cbuffer_charat(cbuf, index);

	if (cp) {
		*cp = c;
	}
}

char cbuffer_getat(cbuffer_t * cbuf, int index)
{
	char * cp = cbuffer_charat(cbuf, index);

	if (cp) {
		return *cp;
	}

	return 0;
}

void cbuffer_addc(cbuffer_t * cbuf, char c)
{
	int newlen = cbuf->len+1;

	if (newlen>=cbuf->capacity) {
		cbuffer_resize(cbuf, (cbuf->capacity) ? cbuf->capacity + cbuf->capacity/2 : 16);
	}

	cbuf->buf[cbuf->len] = c;
	cbuf->len = newlen;
}

void cbuffer_adds(cbuffer_t * cbuf, char * str)
{
	for(int i=0; str[i]; i++) {
		cbuffer_addc(cbuf, str[i]);
	}
}

void cbuffer_addn(cbuffer_t * cbuf, char * str, int len)
{
	for(int i=0; i<len && str[i]; i++) {
		cbuffer_addc(cbuf, str[i]);
	}
}

size_t cbuffer_len(cbuffer_t * cbuf)
{
	return cbuf->len;
}

char * cbuffer_str(cbuffer_t * cbuf)
{
	// Ensure termination
	cbuffer_addc(cbuf, 0);
	char * str = cbuf->buf;

	cbuf->capacity = cbuf->len = 0;
	cbuf->buf = 0;
	
	return str;
}

void cbuffer_trunc(cbuffer_t * cbuf, ssize_t size)
{
	if (size<0) {
		// Truncate up to the last (-size) characters
		size = cbuf->len + size;
	}

	if (size<0) {
		size=0;
	}

	if (size>cbuf->len) {
		size = cbuf->len;
	}

	cbuf->len = size;
}

void cbuffer_test()
{
	cbuffer_t cbuf[1] = {{0}};

	cbuffer_adds(cbuf, "A test string: ");
	cbuffer_adds(cbuf, "Blah blah blah\n\n");
	cbuffer_trunc(cbuf, -1);

	console_writestring(cbuffer_str(cbuf));
}
