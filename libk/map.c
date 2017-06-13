#include "map.h"

#if INTERFACE

#include <stdint.h>

typedef intptr_t map_key;
typedef intptr_t map_data;

typedef void (*walk_func)(void * p, map_key key, map_data data);
typedef void (*walkip_func)(void * p, map_key key, void * data);
typedef void (*walkpp_func)(void * p, void * key, void * data);
typedef void (*walkpi_func)(void * p, void * key, map_data data);

#define MAP_PKEY(key) ((map_key)key)
#define MAP_PDATA(data) ((map_data)data)

#define p2i(p) ((intptr_t)p)
#define i2p(d) ((void *)d)

struct map_ops {
	void (*destroy)( map_t * map );
	void (*walk)( map_t * map, walk_func func, void *p );
	map_data (*put)( map_t * map, map_key key, map_data data );
	map_data (*get)( map_t * map, map_key key );
	map_data (*get_le)( map_t * map, map_key key );
	map_data (*remove)( map_t * map, map_key key );
	void (*optimize)(map_t * map);
	iterator_t * (*iterator)( map_t * map );
};

typedef struct map {
	struct map_ops * ops;
} map_t;

#endif

void map_destroy( map_t * map )
{
	map->ops->destroy(map);
	slab_free(map);
}

struct walk_wrapper
{
	union {
		void * f;
		walkip_func walkip;
		walkpi_func walkpi;
		walkpp_func walkpp;
	} f;
	void * p;
};

static void walk_walkip_func( void * p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->f.walkip(w->p, key, (void*)data);
}

static void walk_walkpp_func( void * p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->f.walkpp(w->p, (void*)key, (void*)data);
}

static void walk_walkpi_func( void * p, map_key key, map_data data )
{
	struct walk_wrapper * w = (struct walk_wrapper*)p;
	w->f.walkpi(w->p, (void*)key, data);
}

void map_walkip( map_t * map, walkip_func func, void * p )
{
	struct walk_wrapper wrapper = {
		{ func },
		p
	};
	map->ops->walk(map, walk_walkip_func, &wrapper);
}

void map_walkpp( map_t * map, walkpp_func func, void * p )
{
	struct walk_wrapper wrapper = {
		{ func },
		p
	};
	map->ops->walk(map, walk_walkpp_func, &wrapper);
}

void map_walkpi( map_t * map, walkpi_func func, void * p )
{
	struct walk_wrapper wrapper = {
		{ func },
		p
	};
	map->ops->walk(map, walk_walkpi_func, &wrapper);
}

void map_walk( map_t * map, walk_func func, void * p )
{
	map->ops->walk(map, func, p);
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
	return map->ops->get(map, key);
}

map_data map_getpi( map_t * map, void * key )
{
	return map->ops->get(map, (map_key)key);
}

void * map_getip( map_t * map, map_key key )
{
	return (void*)map->ops->get(map, key);
}

void * map_getpp( map_t * map, void * key )
{
	return (void*)map->ops->get(map, (map_key)key);
}

map_data map_get_le( map_t * map, map_key key )
{
	return map->ops->get_le(map, key);
}

map_data map_getpi_le( map_t * map, void * key )
{
	return map->ops->get_le(map, (map_key)key);
}

void * map_getip_le( map_t * map, map_key key )
{
	return (void*)map->ops->get_le(map, key);
}

void * map_getpp_le( map_t * map, void * key )
{
	return (void*)map->ops->get_le(map, (map_key)key);
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
