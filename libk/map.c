#include "map.h"

#if INTERFACE

typedef void (*walk_func)(void * data);

struct map_ops {
        void (*destroy)( map * map );
        void (*walk)( map * map, walk_func func );

        void * (*put)( map * map, void * key, void * data );
        void * (*get)( map * map, void * key );
        void * (*remove)( map * map, void * key );

        iterator (*iterator)( map * map, int keys );
};

typedef struct {
	struct map_ops * ops;
} map;

#endif

void map_destroy( map * map )
{
        map->ops->destroy(map);
        slab_free(map);
}

void map_walk( map * map, walk_func func )
{
        map->ops->walk(map, func);
}

void * map_put( map * map, void * key, void * data )
{
        return map->ops->put(map, key, data);
}

void * map_get( map * map, void * key )
{
        return map->ops->get(map, key);
}

void * map_remove( map * map, void * key )
{
        return map->ops->remove(map, key);
}

iterator map_iterator( map * map, int keys )
{
        return map->ops->iterator(map, keys);
}
