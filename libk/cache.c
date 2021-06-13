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

static map_data cache_put( const map_t * map, map_key key, map_data data )
{
	cache_t * cache = container_of(map, cache_t, map);
	cache_node_t * node = malloc(sizeof(*node));

	node->key = key;
	node->data = data;
	LIST_APPEND(cache->cold, node);

	return map_put(cache->backing, key, (map_data)node);
}

static map_data cache_get( const map_t * map, map_key key, map_eq_test cond )
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
	const void * const p;
} cache_walk_t;

static void cache_walk_wrap( const void * const p, map_key key, map_data data)
{
	const cache_walk_t * wrapper = p;
	cache_node_t * node = (void*)data;
	wrapper->func(wrapper->p, node->key, node->data);
}

static void cache_walk( const map_t * map, walk_func func, const void * const p )
{
	cache_walk_t wrapper = { .func = func, .p = p };
	cache_t * cache = container_of(map, cache_t, map);
	map_walk(cache->backing, cache_walk_wrap, &wrapper);
}

static void cache_walk_range( const map_t * map, walk_func func, const void * const p, map_key from, map_key to )
{
	cache_walk_t wrapper = { .func = func, .p = p };
	cache_t * cache = container_of(map, cache_t, map);
	map_walk_range(cache->backing, cache_walk_wrap, &wrapper, from, to);
}

static void cache_optimize(const map_t * map)
{
	cache_t * cache = container_of(map, cache_t, map);
	map_optimize(cache->backing);
}

static map_data cache_remove( const map_t * map, map_key key )
{
	cache_t * cache = container_of(map, cache_t, map);
	cache_node_t * node = map_removeip(cache->backing, key);
	if (node) {
		cache_remove_node(cache, node);
		return node->data;
	}

	return 0;
}

static interface_map_t cache_t_map [] =
{
	INTERFACE_MAP_ENTRY(cache_t, iid_map_t, map),
};
static INTERFACE_IMPL_QUERY(map_t, cache_t, map)
static INTERFACE_OPS_TYPE(map_t) INTERFACE_IMPL_NAME(map_t, cache_t) = {
        INTERFACE_IMPL_QUERY_METHOD(map_t, cache_t)
        INTERFACE_IMPL_METHOD(walk, cache_walk)
        INTERFACE_IMPL_METHOD(walk_range, cache_walk_range)
        INTERFACE_IMPL_METHOD(put, cache_put)
        INTERFACE_IMPL_METHOD(get, cache_get)
        INTERFACE_IMPL_METHOD(optimize, cache_optimize)
        INTERFACE_IMPL_METHOD(remove, cache_remove)
};

map_t * cache_new(int (*comp)(map_key k1, map_key k2))
{
	cache_t * cache = calloc(1, sizeof(*cache));
	cache->map.ops = &cache_t_map_t;
	cache->backing = splay_new(comp);

	return com_query(cache_t_map, iid_map_t, cache);
}


void cache_test()
{
	map_t * cache = cache_new(map_strcmp);

	map_test(cache, 0);
}
