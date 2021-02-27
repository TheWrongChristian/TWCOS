#include "map.h"

#if INTERFACE

#include <stdint.h>
#include <stdarg.h>

typedef uintptr_t map_key;
typedef intptr_t map_data;

typedef void (*walk_func)(const void * const p, map_key key, map_data data);
typedef void (*walkip_func)(const void * const p, map_key key, void * data);
typedef void (*walkpp_func)(const void * const p, void * key, void * data);
typedef void (*walkpi_func)(const void * const p, void * key, map_data data);

typedef int (*prefix_func)(map_key prefix, map_key key);
typedef int (*prefixp_func)(void * prefix, void * key);

#define MAP_PKEY(key) ((map_key)key)
#define MAP_PDATA(data) ((map_data)data)

#define p2i(p) ((intptr_t)p)
#define i2p(d) ((void *)d)

struct map_ops {
	void (*destroy)( const map_t * map );
	void (*walk)( const map_t * map, const walk_func func, const void *p );
	void (*walk_range)( const map_t * map, const walk_func func, const void *p, const map_key from, const map_key to );
	map_data (*put)( const map_t * map, const map_key key, const map_data data );
	map_data (*get)( const map_t * map, const map_key key, const map_eq_test cond );
	map_data (*remove)( const map_t * map, const map_key key );
	void (*optimize)(const map_t * map);
	iterator_t * (*iterator)( const map_t * map );
};

typedef struct map_s {
	struct map_ops * ops;
} map_t;

enum map_eq_test { MAP_LT, MAP_LE, MAP_EQ, MAP_GE, MAP_GT };

#if 0
typedef struct map_compound_key_t map_compound_key_t;
#endif
struct map_compound_key_t {
	size_t buflen;
	char buf[];
};

struct map_compound_tempkey_t {
	size_t buflen;
	char buf[24];
};

#endif

void map_destroy( map_t * map )
{
	map->ops->destroy(map);
}

struct walk_wrapper
{
	union {
		walk_func walk;
		walkip_func walkip;
		walkpi_func walkpi;
		walkpp_func walkpp;
	};
	union {
		prefix_func prefix;
		prefixp_func prefixp;
	};
	union {
		map_key key_prefix;
		void * key_prefixp;
	};
	const void * const p;
};

static void walk_walkip_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->walkip(w->p, key, (void*)data);
}

static void walk_walkpp_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->walkpp(w->p, (void*)key, (void*)data);
}

static void walk_walkpi_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->walkpi(w->p, (void*)key, data);
}

static void walk_walk_prefix_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	if (w->prefix(w->key_prefix, key)) {
		w->walk(w->p, key, data);
	}
}

static void walk_walkip_prefix_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	if (w->prefix(w->key_prefix, key)) {
		w->walkip(w->p, key, (void*)data);
	}
}

static void walk_walkpp_prefix_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	if (w->prefixp(w->key_prefixp, (void*)key)) {
		w->walkpp(w->p, (void*)key, (void*)data);
	}
}

static void walk_walkpi_prefix_func( const void * const p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	if (w->prefixp(w->key_prefixp, (void*)key)) {
		w->walkpi(w->p, (void*)key, data);
	}
}

void map_walkip( map_t * const map, walkip_func func, const void * const p )
{
	struct walk_wrapper wrapper = {
		walkip: func,
		p: p
	};
	map->ops->walk(map, walk_walkip_func, &wrapper);
}

void map_walkpp( map_t * map, walkpp_func func, void * p )
{
	struct walk_wrapper wrapper = {
		walkpp: func,
		p: p
	};
	map->ops->walk(map, walk_walkpp_func, &wrapper);
}

void map_walkpi( map_t * map, walkpi_func func, void * p )
{
	struct walk_wrapper wrapper = {
		walkpi: func,
		p: p,
	};
	map->ops->walk(map, walk_walkpi_func, &wrapper);
}

void map_walk( map_t * map, walk_func func, void * p )
{
	map->ops->walk(map, func, p);
}

void map_walkip_range( map_t * map, walkip_func func, void * p, map_key from, map_key to )
{
	struct walk_wrapper wrapper = {
		walkip: func,
		p: p,
	};
	map->ops->walk_range(map, walk_walkip_func, &wrapper, from, to);
}

void map_walkpp_range( map_t * map, walkpp_func func, void * p, void * from, void * to )
{
	struct walk_wrapper wrapper = {
		walkpp: func,
		p: p,
	};
	map->ops->walk_range(map, walk_walkpp_func, &wrapper, (map_key)from, (map_key)to);
}

void map_walkpi_range( map_t * map, walkpi_func func, void * p, void * from, void * to )
{
	struct walk_wrapper wrapper = {
		walkpi: func,
		p: p,
	};
	map->ops->walk_range(map, walk_walkpi_func, &wrapper, (map_key)from, (map_key)to);
}

void map_walk_range( map_t * map, walk_func func, void * p, map_key from, map_key to )
{
	map->ops->walk_range(map, func, p, from, to);
}

void map_walk_prefix( map_t * map, walk_func func, void * p, prefix_func prefix_func, map_key prefix)
{
	struct walk_wrapper wrapper = {
		walk: func,
		prefix: prefix_func,
		key_prefix: prefix,
		p: p,
	};
	map_key from = prefix;
	map->ops->walk_range(map, walk_walk_prefix_func, &wrapper, from, 0);
}

void map_walkip_prefix( map_t * map, walkpp_func func, void * p, prefix_func prefix_func, map_key prefix)
{
	struct walk_wrapper wrapper = {
		walk: (walk_func)func,
		prefix: prefix_func,
		key_prefix: prefix,
		p: p,
	};
	map_key from = prefix;
	map->ops->walk_range(map, walk_walkip_prefix_func, &wrapper, from, 0);
}

void map_walkpi_prefix( map_t * map, walkpi_func func, void * p, prefixp_func prefix_func, void * prefix)
{
	struct walk_wrapper wrapper = {
		walk: (walk_func)func,
		prefixp: prefix_func,
		key_prefixp: prefix,
		p: p,
	};
	void * from = prefix;
	map->ops->walk_range(map, walk_walkpi_prefix_func, &wrapper, (map_data)from, (map_data)0);
}

void map_walkpp_prefix( map_t * map, walkpp_func func, void * p, prefixp_func prefix_func, void * prefix)
{
	struct walk_wrapper wrapper = {
		walk: (walk_func)func,
		prefixp: prefix_func,
		key_prefixp: prefix,
		p: p,
	};
	void * from = prefix;
	map->ops->walk_range(map, walk_walkpp_prefix_func, &wrapper, (map_data)from, (map_data)0);
}

map_data map_put( map_t * map, map_key key, map_data data )
{
	return map->ops->put(map, key, data);
}

map_data map_putpi( map_t * map, void * key, map_data data )
{
	return map->ops->put(map, (map_key)key, data);
}

void * map_putip( map_t * map, map_key key, void * data )
{
	return (void*)map->ops->put(map, key, (map_data)data);
}

void * map_putpp( map_t * map, void * key, void * data )
{
	return (void*)map->ops->put(map, (map_key)key, (map_data)data);
}

map_data map_get( map_t * map, map_key key )
{
	return map->ops->get(map, key, MAP_EQ);
}

map_data map_getpi( map_t * map, void * key )
{
	return map->ops->get(map, (map_key)key, MAP_EQ);
}

void * map_getip( map_t * map, map_key key )
{
	return (void*)map->ops->get(map, key, MAP_EQ);
}

void * map_getpp( map_t * map, void * key )
{
	return (void*)map->ops->get(map, (map_key)key, MAP_EQ);
}

map_data map_get_cond( map_t * map, map_key key, map_eq_test cond )
{
	return map->ops->get(map, key, cond);
}

map_data map_getpi_cond( map_t * map, void * key, map_eq_test cond )
{
	return map->ops->get(map, (map_key)key, cond);
}

void * map_getip_cond( map_t * map, map_key key, map_eq_test cond )
{
	return (void*)map->ops->get(map, key, cond);
}

void * map_getpp_cond( const map_t * map, const void * key, const map_eq_test cond )
{
	return (void*)map->ops->get(map, (map_key)key, cond);
}

map_data map_remove( map_t * map, map_key key )
{
	return map->ops->remove(map, key);
}

map_data map_removepi( map_t * map, void * key )
{
	return map->ops->remove(map, (map_key)key);
}

void * map_removepp( map_t * map, void * key )
{
	return (void*)map->ops->remove(map, (map_key)key);
}

void * map_removeip( map_t * map, map_key key )
{
	return (void*)map->ops->remove(map, key);
}

void map_optimize(map_t * map)
{
	map->ops->optimize(map);
}

iterator_t * map_iterator( map_t * map)
{
        return map->ops->iterator(map);
}

static void map_walk_dump(const void * const p, void * key, void * data)
{
        kernel_printk("%s\n", data);

        if (p) {
                map_t * akmap = (map_t*)p;
                /* Add the data to the ak map */
                map_key * ak = (map_key*)map_arraykey2((intptr_t)akmap, *((char*)data));
                map_putpp(akmap, ak, data);
        }
}

int map_strcmp(map_key k1, map_key k2)
{
	return strcmp((char*)k1, (char*)k2);
}

int map_keycmp(map_key k1, map_key k2)
{
	return (k1>k2) ? 1 : (k2>k1) ? -1: 0;
}

int map_arraycmp(map_key k1, map_key k2)
{
	map_key * a1 = (map_key*)k1;
	map_key * a2 = (map_key*)k2;

	while(*a1 && *a2 && *a1 == *a2) {
		a1++;
		a2++;
	}

	return map_keycmp(*a1, *a2);
}

map_key map_arraykey1( map_key k )
{
	map_key * key = malloc(sizeof(*key)*2);
	key[0] = k;
	key[1] = 0;

	return (map_key)key;
}

map_key map_arraykey2( map_key k1, map_key k2 )
{
	map_key * key = malloc(sizeof(*key)*3);
	key[0] = k1;
	key[1] = k2;
	key[2] = 0;

	return (map_key)key;
}

map_key map_arraykey3( map_key k1, map_key k2, map_key k3 )
{
	map_key * key = malloc(sizeof(*key)*4);
	key[0] = k1;
	key[1] = k2;
	key[2] = k3;
	key[3] = 0;

	return (map_key)key;
}

#if 0
static void map_compound_key_add( map_compound_key_t * key, void * p, size_t size)
{
}
#endif

static size_t map_compound_key_process( map_compound_key_t * key, const char * fmt, va_list ap)
{
	const char * f = fmt;
	int len = 0;
	char * buf = (key) ? key->buf : 0;

	/* Initialize temporary buffer */
	if (key && 0 == key->buflen) {
		map_compound_tempkey_t * tkey = (map_compound_tempkey_t*)key;
		tkey->buflen = sizeof(tkey->buf);
	}

	while(*f) {
		if ('i' == *f) {
			f++;
			switch(*f) {
				uint32_t i32;
				uint64_t i64;
			case '4':
				i32 = va_arg(ap, uint32_t);
				if (buf) {
					*buf++=(i32>>24) & 0xff;
					*buf++=(i32>>16) & 0xff;
					*buf++=(i32>>8) & 0xff;
					*buf++=(i32) & 0xff;
				}
				len += 4;
				break;
			case '8':
				i64 = va_arg(ap, int64_t);
				if (buf) {
					*buf++=(i64>>56) & 0xff;
					*buf++=(i64>>48) & 0xff;
					*buf++=(i64>>40) & 0xff;
					*buf++=(i64>>32) & 0xff;
					*buf++=(i64>>24) & 0xff;
					*buf++=(i64>>16) & 0xff;
					*buf++=(i64>>8) & 0xff;
					*buf++=(i64) & 0xff;
				}
				len += 8;
				break;
			default:
				kernel_panic("Unknown key specifier at position %d: %s\n", f-fmt, fmt);
				break;
			}
		} else if ('p' == *f) {
			uintptr_t p = va_arg(ap, uintptr_t);
			for(int i=sizeof(p)*8; i>0; i-=8) {
				len++;
				if (buf) {
					*buf++=(p>>(i-8));
				}
			}
		} else if ('s' == *f) {
			char * s = va_arg(ap, char *);
			if (buf) {
				int copylen = key->buflen - len;
				memcpy(buf, s, copylen);

				return key->buflen;
			}
			len += strlen(s)+1;
		} else {
			kernel_panic("Unknown key specifier at position %d: %s\n", f-fmt, fmt);
		}
		f++;
	}

	if (key) {
		key->buflen = len;
	}

	return sizeof(*key)+len;
}

map_compound_key_t * map_compound_key( const char * fmt, ... )
{
	va_list ap;

	va_start(ap, fmt);
	size_t keylen = map_compound_key_process(0, fmt, ap);
	va_end(ap);
	map_compound_key_t * key = malloc(keylen);
	key->buflen = keylen-sizeof(*key);
	va_start(ap, fmt);
	map_compound_key_process(key, fmt, ap);
	va_end(ap);

	return key;
}

map_compound_key_t * map_compound_tkey( const char * fmt, ... )
{
	va_list ap;

	va_start(ap, fmt);
	size_t keylen = map_compound_key_process(0, fmt, ap);
	va_end(ap);
	map_compound_key_t * key = tmalloc(keylen);
	key->buflen = keylen-sizeof(*key);
	va_start(ap, fmt);
	map_compound_key_process(key, fmt, ap);
	va_end(ap);

	return key;
}

map_compound_key_t * map_compound_tempkey( map_compound_tempkey_t * key, const char * fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	map_compound_key_process((map_compound_key_t*)key, fmt, ap);
	va_end(ap);

	return (map_compound_key_t*)key;
}

int map_compound_key_comp(map_compound_key_t * key1, map_compound_key_t * key2)
{
	if (key1->buflen < key2->buflen) {
		int d = memcmp(key1->buf, key2->buf, key1->buflen);

		return (d) ? d : -1;
	} else if (key1->buflen > key2->buflen) {
		int d = memcmp(key1->buf, key2->buf, key2->buflen);

		return (d) ? d : 1;
	} else {
		return memcmp(key1->buf, key2->buf, key1->buflen);
	}
}

int map_compound_key_prefix(map_compound_key_t * prefix, map_compound_key_t * key)
{
	if (prefix->buflen > key->buflen) {
		return 0;
	}

	return (0 == memcmp(prefix->buf, key->buf, prefix->buflen));
}

void map_test(map_t * map, map_t * akmap)
{
	static char * data[] = {
		"Jonas",
		"Christmas",
		"This is a test string",
		"Another test string",
		"Mary had a little lamb",
		"Supercalblahblahblah",
		"Zanadu",
		"Granny",
		"Roger",
		"Steve",
		"Rolo",
		"MythTV",
		"Daisy",
		"Thorntons",
		"Humbug",
	};

	for( int i=0; i<(sizeof(data)/sizeof(data[0])); i++) {
		map_putpp(map, data[i], data[i]);
	}

	map_walkpp(map, map_walk_dump, akmap);
	map_walkpp_range(map, map_walk_dump, 0, "Christ", "Steven");
	if (akmap) {
		map_walkpp_range(akmap, map_walk_dump, 0, (void*)map_arraykey1((map_key)akmap), (void*)map_arraykey1((map_key)akmap+1));
	}

	kernel_printk("%s LE Christ\n", map_getpp_cond(map, "Christ", MAP_LE));
	kernel_printk("%s EQ Christmas\n", map_getpp(map, "Christmas"));

	for( int i=0; i<(sizeof(data)/sizeof(data[0])); i++) {
		map_removepp(map, data[i]);
	}

#if 0
	map_compound_key_t * key1 = map_compound_key("i4i8s", (int32_t)10, (int64_t)32, "blah");
	map_compound_tempkey_t prefix = {0};
	map_compound_key_t * key2 = map_compound_tempkey(&prefix, "i4i8", (int32_t)10, (int64_t)32);
	int comp = map_compound_key_comp(key1, key2);
	int isprefix = map_compound_key_prefix(key2, key1);
	// kernel_printk("comp=%d, isprefix=%s\n", comp, (isprefix) ? "true" : "false");
#endif
}

static void map_put_all_walk(const void *const p,map_key key,map_data data)
{
	map_t * to = (map_t *)p;

	map_put(to, key, data);
}

void map_put_all( map_t * to, map_t * from )
{
	map_walk(from, map_put_all_walk, to);
}
