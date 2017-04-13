#include "map.h"

#if INTERFACE

#include <stdint.h>

typedef void (*walk_func)(void * p, map_key key, intptr_t data);
typedef void (*walkp_func)(void * p, map_key key, void * data);

typedef uintptr_t map_key;
#define MAP_PKEY(key) ((map_key)key)
#define MAP_PKEY(key) ((map_key)key)

struct map_ops {
        void (*destroy)( map_t * map );
        void (*walk)( map_t * map, walk_func func, void *p );

        void * (*put)( map_t * map, map_key key, intptr_t data );
        void * (*get)( map_t * map, map_key key );
        void * (*get_le)( map_t * map, map_key key );
        void * (*remove)( map_t * map, map_key key );

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

struct walkp_wrapper
{
	walkp_func func;
	void * p;
};

static void walk_walkp_func( void * p, map_key key, intptr_t data )
{
	struct walkp_wrapper * w = (struct walkp_wrapper*)p;
	w->func(w->p, key, (void*)data);
}

void map_walkp( map_t * map, walkp_func func, void * p )
{
	struct walkp_wrapper wrapper = {
		func,
		p
	};
	map->ops->walk(map, walk_walkp_func, &wrapper);
}

void map_walk( map_t * map, walk_func func, void * p )
{
	map->ops->walk(map, func, p);
}

void * map_put( map_t * map, map_key key, intptr_t data )
{
	return map->ops->put(map, key, data);
}

void * map_putp( map_t * map, map_key key, void * data )
{
	return map->ops->put(map, key, (intptr_t)data);
}

intptr_t map_get( map_t * map, map_key key )
{
	return map->ops->get(map, key);
}

void * map_getp( map_t * map, map_key key )
{
	return (void*)map->ops->get(map, key);
}

void * map_get_le( map_t * map, map_key key )
{
	return map->ops->get_le(map, key);
}

intptr_t map_remove( map_t * map, map_key key )
{
	return map->ops->remove(map, key);
}

void * map_removep( map_t * map, map_key key )
{
	return map->ops->remove(map, key);
}

void map_optimize(map_t * map)
{
	map->ops->optimize(map);
}

iterator_t * map_iterator( map_t * map)
{
        return map->ops->iterator(map);
}
