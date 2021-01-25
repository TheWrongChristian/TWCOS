#include "cache.h"

#if INTERFACE


#endif


typedef struct cache_node_t {
	map_key key;
	map_data data;

	struct cache_node_t * next;
	struct cache_node_t * prev;
} cache_node_t;

typedef struct cache_t {
	map_t map;

	map_t * backing;

	struct cache_node_t * cold;
	struct cache_node_t * hot;
} cache_t;

static void cache_remove_node(cache_t * cache, cache_node_t * node)
{
	if (node == cache->hot) {
		LIST_DELETE(cache->hot, node);
	} else {
		LIST_DELETE(cache->cold, node);
	}
}

static map_data cache_put( map_t * map, map_key key, map_data data )
{
	cache_t * cache = container_of(map, cache_t, map);
	cache_node_t * node = malloc(sizeof(*node));

	node->key = key;
	node->data = data;
	LIST_APPEND(cache->cold, node);

	return map_put(cache->backing, key, (map_data)node);
}

static map_data cache_get( map_t * map, map_key key, map_eq_test cond )
{
	cache_t * cache = container_of(map, cache_t, map);
	cache_node_t * node = map_getip_cond(cache->backing, key, cond);

	if (node) {
		/* Key exists, move it to the hot queue */
		cache_remove_node(cache, node);
		LIST_APPEND(cache->hot, node);

		return node->data;
	}

	return 0;
}

typedef struct {
	walk_func func;
	void * p;
} cache_walk_t;

static void cache_walk_wrap( void * p, map_key key, map_data data)
{
	cache_walk_t * wrapper = p;
	cache_node_t * node = (void*)data;
	wrapper->func(wrapper->p, node->key, node->data);
}

static void cache_walk( map_t * map, walk_func func, void * p )
{
	cache_walk_t wrapper = { .func = func, .p = p };
	cache_t * cache = container_of(map, cache_t, map);
	map_walk(cache->backing, cache_walk_wrap, &wrapper);
}

static void cache_walk_range( map_t * map, walk_func func, void * p, map_key from, map_key to )
{
	cache_walk_t wrapper = { .func = func, .p = p };
	cache_t * cache = container_of(map, cache_t, map);
	map_walk_range(cache->backing, cache_walk_wrap, &wrapper, from, to);
}

static void cache_optimize(map_t * map)
{
	cache_t * cache = container_of(map, cache_t, map);
	map_optimize(cache->backing);
}

static map_data cache_remove( map_t * map, map_key key )
{
	cache_t * cache = container_of(map, cache_t, map);
	cache_node_t * node = map_removeip(cache->backing, key);
	if (node) {
		cache_remove_node(cache, node);
		return node->data;
	}

	return 0;
}

map_t * cache_new(int (*comp)(map_key k1, map_key k2))
{
	static struct map_ops cache_ops = {
		walk: cache_walk,
		walk_range: cache_walk_range,
		put: cache_put,
		get: cache_get,
		optimize: cache_optimize,
		remove: cache_remove,
	};
	cache_t * cache = calloc(1, sizeof(*cache));
	cache->map.ops = &cache_ops;
	cache->backing = splay_new(comp);

	return &cache->map;
}


void cache_test()
{
	map_t * cache = cache_new(strcmp);

	map_test(cache, 0);
}
