#include "arraymap.h"

exception_def ArrayMapFullException = { "ArrayMapFullException", &Exception };

typedef struct arraymap_s {
	map_t map;

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

static int arraymap_get_index(arraymap_t * amap, map_key key, map_eq_test cond )
{
	int low = 0;
	int high = amap->count;
	
	while(1) {
		int i = (low + high) / 2;
		intptr_t diff = amap->comp(key, amap->data[i].key);

		if (diff<0) {
			high = i;
			if (low == high-1) {
				switch(cond) {
				case MAP_GT: case MAP_GE:
					return i;
				case MAP_LT: case MAP_LE:
					return i-1;
				default:
					return -1;
				}
			}
		} else if (diff>0) {
			low = i;
			if (low == high-1) {
				switch(cond) {
				case MAP_GT: case MAP_GE:
					return (i<amap->count) ? i+1 : -1;
				case MAP_LT: case MAP_LE:
					return i;
				default:
					return -1;
				}
			}
		} else {
			switch(cond) {
			case MAP_LT:
				return i-1;
			case MAP_GT:
				return (i<amap->count) ? i+1 : -1;
			default:
				return i;
			}
		}
	}

	return -1;
}

static void arraymap_walk(map_t * map, walk_func func, void * p )
{
	arraymap_t * amap = container_of(map, arraymap_t, map);

	for(int i=0; i<amap->count; i++) {
		func(p, amap->data[i].key, amap->data[i].data);
	}
}

static void arraymap_walk_range(map_t * map, walk_func func, void * p, map_key from, map_key to )
{
	arraymap_t * amap = container_of(map, arraymap_t, map);
	int indexfrom = arraymap_get_index(amap, from, MAP_GE);
	int indexto = arraymap_get_index(amap, to, MAP_LT);

	for(int i=indexfrom; i<=indexto; i++) {
		func(p, amap->data[i].key, amap->data[i].data);
	}
}

static map_data arraymap_put( map_t * map, map_key key, map_data data )
{
	arraymap_t * amap = container_of(map, arraymap_t, map);

	if (amap->count) {
		int low = 0;
		int high = amap->count;
		int diff;
		do {
			int i = (low + high) / 2;
			diff = amap->comp(key, amap->data[i].key);

			if (diff<0) {
				high = i;
			} else if (diff>0) {
				low = i;
			} else {
				/* Replace existing data */
				map_data old = amap->data[i].data;
				amap->data[i].data = data;
				return old;
			}
		} while(low<high-1);

		if (amap->count == amap->capacity) {
			/* Full! */
			/* FIXME: Put the exception here */
			KTHROWF(ArrayMapFullException, "Array Map full - capacity %d", amap->capacity);
		}
		amap->count++;

		int insert = low;
		diff = amap->comp(key, amap->data[low].key);
		if (diff>0) {
			/* Insert after low index */
			insert = low+1;
		}

		/* new data goes in at "insert", existing data is shuffled along */
		map_key new_key = key;
		map_data new_data = data;
		for(int i=insert; i<amap->count; i++) {
			map_key temp_key = amap->data[i].key;
			map_data temp_data = amap->data[i].data;

			amap->data[i].key = new_key;
			amap->data[i].data = new_data;

			new_key = temp_key;
			new_data = temp_data;
		}
	} else {
		/* Previously empty */
		amap->count=1;
		amap->data[0].key = key;
		amap->data[0].data = data;
	}

	return 0;
}

static map_data arraymap_get( map_t * map, map_key key, map_eq_test cond )
{
	arraymap_t * amap = container_of(map, arraymap_t, map);
	int i = arraymap_get_index(amap, key, cond);

	if (i>=0) {
		return amap->data[i].data;
	}

	/* Not found */
	return 0;
}

static map_data arraymap_remove( map_t * map, map_key key )
{
	arraymap_t * amap = container_of(map, arraymap_t, map);
	int i = arraymap_get_index(amap, key, MAP_EQ);

	if (i>=0) {
		map_data old = amap->data[i].data;
		amap->count--;
		for(; i<amap->count; i++) {
			amap->data[i].key = amap->data[i+1].key;
			amap->data[i].data = amap->data[i+1].data;
		}

		/* Remove stale references for GC */
		amap->data[i].key = amap->data[i].data = 0;

		return old;
	}

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

	map = calloc(1, size);

	map->map.ops = &arraymap_ops;
	map->capacity = capacity;
	map->comp = (comp) ? comp : map_keycmp;
	map->count = 0;

	return &map->map;
}


void arraymap_test()
{
	map_t * map = arraymap_new(map_strcmp, 20);
	map_t * akmap = arraymap_new(map_arraycmp, 20);
	map_test(map, akmap);
}
