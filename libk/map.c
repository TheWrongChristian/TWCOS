#include "map.h"

#if INTERFACE

typedef void (*walk_func)(void * data);

struct map_ops {
        void (*destroy)( map_t * map );
        void (*walk)( map_t * map, walk_func func );

        void * (*put)( map_t * map, void * key, void * data );
        void * (*get)( map_t * map, void * key );
        void * (*remove)( map_t * map, void * key );

        iterator_t * (*iterator)( map_t * map, int keys );
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

void map_walk( map_t * map, walk_func func )
{
        map->ops->walk(map, func);
}

void * map_put( map_t * map, void * key, void * data )
{
        return map->ops->put(map, key, data);
}

void * map_get( map_t * map, void * key )
{
        return map->ops->get(map, key);
}

void * map_remove( map_t * map, void * key )
{
        return map->ops->remove(map, key);
}

iterator_t * map_iterator( map_t * map, int keys )
{
        return map->ops->iterator(map, keys);
}
