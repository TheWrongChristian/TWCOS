#include "arraymap.h"


typedef struct arraymap_s {
	struct map_ops * ops;

	int capacity;
	int count;

	int (*comp)(map_key k1, map_key k2);

	struct {
		map_key key;
		map_data data;
	} data[0];
} arraymap_t;


static void arraymap_destroy(map_t * map)
{
}

static void arraymap_walk(map_t * map, walk_func func, void * p )
{
}

static void arraymap_walk_range(map_t * map, walk_func func, void * p, map_key from, map_key to )
{
}

static map_data arraymap_put( map_t * map, map_key key, map_data data )
{
	return 0;
}

static map_data arraymap_get( map_t * map, map_key key, map_eq_test cond )
{
	arraymap_t * amap = (arraymap_t*)map;
	int low = 0;
	int high = amap->count;

	while(low<high) {
		int i = (low + high) / 2;
		intptr_t diff = (amap->comp) ? amap->comp(key, amap->data[i].key) : key - amap->data[i].key;

		if (diff<0) {
			high = i;
		} else if (diff>0) {
			low = i;
		} else {
			return amap->data[i].data;
		}
	}

	/* Not found */
	return 0;
}

static map_data arraymap_remove( map_t * map, map_key key )
{
	return 0;
}

map_t * arraymap_new(int (*comp)(map_key k1, map_key k2), int capacity)
{
	static struct map_ops arraymap_ops = {
		destroy: arraymap_destroy,
		walk: arraymap_walk,
		walk_range: arraymap_walk_range,
		put: arraymap_put,
		get: arraymap_get,
		optimize: 0,
		remove: arraymap_remove,
		iterator: 0
	};
	arraymap_t * map = 0;
	int size = sizeof(*map) + capacity * sizeof(map->data[0]);

	map = malloc(size);

	map->ops = &arraymap_ops;
	map->capacity = capacity;
	map->comp = comp;
	map->count = 0;

	return (map_t*)map;
}
