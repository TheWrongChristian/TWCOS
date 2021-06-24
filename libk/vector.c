#include "vector.h"

#if INTERFACE

#include <stdint.h>

#endif

#define VECTOR_TABLE_ENTRIES_LOG2 6
#define VECTOR_TABLE_ENTRIES (1<<VECTOR_TABLE_ENTRIES_LOG2)

typedef struct vector_table_t vector_table_t;
typedef struct vector_t vector_t;

struct vector_table_t {
	int level;

	intptr_t d[VECTOR_TABLE_ENTRIES];
};

struct vector_t {
	map_t map;
	vector_table_t * table;
};

static slab_type_t vectors[1] = {SLAB_TYPE(sizeof(vector_t), 0, 0)};
static slab_type_t tables[1] = {SLAB_TYPE(sizeof(vector_table_t), 0, 0)};

void vector_init()
{
	INIT_ONCE();
}

static vector_table_t * vector_table_new(int level)
{
	vector_table_t * table = slab_calloc(tables);

	table->level = level;

	return table;
}

static intptr_t * vector_entry_get(vector_table_t * table, map_key i, int create)
{
	if (table->level) {
		int shift = VECTOR_TABLE_ENTRIES_LOG2*table->level;
#if 0
		if (shift>sizeof(uint32_t)*4) {
			kernel_panic("Vector overflow!");
		}
#endif
		int index = i>>shift;
		if (VECTOR_TABLE_ENTRIES <= index) {
			/* Beyond the bounds of the vector */
			return 0;
		} else if (0 == table->d[index]) {
			if (create) {
				table->d[index] = (intptr_t)vector_table_new(table->level-1);
			} else {
				return 0;
			}
		}

		return vector_entry_get((vector_table_t *)table->d[index], i&((1<<shift)-1), create);
	} else if (i<VECTOR_TABLE_ENTRIES) {
		return table->d+i;
	} else if (!create) {
		return 0;
	} else {
		kernel_panic("Vector leaf overflow");
	}
}

static void vector_checksize(vector_t * v, map_key i)
{
	/* Extend the table as necessary */
	while(1<<(VECTOR_TABLE_ENTRIES_LOG2*(v->table->level+1))<=i) {
		vector_table_t * table = vector_table_new(v->table->level+1);
		table->d[0] = (intptr_t)v->table;
		v->table = table;
	}
}

static map_data vector_put(const map_t * m, map_key i, map_data d)
{
	vector_t * v = container_of(m, vector_t, map);
	vector_checksize(v, i);
	intptr_t * entry = vector_entry_get(v->table, i, 1);
	intptr_t old = *entry;
	*entry = d;
	return old;
}

static map_data vector_get(const map_t * m, map_key i, map_eq_test cond)
{
	vector_t * v = container_of(m, vector_t, map);
	intptr_t * entry = vector_entry_get(v->table, i, 0);

	if (entry) {
		return *entry;
	}

	return 0;
}

static void vector_walk_table(vector_t * v, vector_table_t * t, const void * arg, int base, walk_func f)
{
	for(int i=0; i<VECTOR_TABLE_ENTRIES; i++) {
		if (t->d[i]) {
			if (t->level) {
				vector_walk_table(v, (vector_table_t *)t->d[i], arg, (base+i)*VECTOR_TABLE_ENTRIES, f);
			} else {
				f(arg, base+i, t->d[i]);
			}
		}
	}
}

static void vector_walk(const map_t * m, const walk_func f, const void * arg )
{
	vector_t * v = container_of(m, vector_t, map);
	if (v->table) {
		vector_walk_table(v, v->table, arg, 0, f);
	}
}

static void vector_test_walk(const void * const ignored, map_key i, void * p)
{
	kernel_printk("v[%d] = %p\n", i, p);
}

static interface_map_t vector_t_map [] =
{
        INTERFACE_MAP_ENTRY(vector_t, iid_map_t, map),
};
static INTERFACE_IMPL_QUERY(map_t, vector_t, map)
static INTERFACE_OPS_TYPE(map_t) INTERFACE_IMPL_NAME(map_t, vector_t) = {
        INTERFACE_IMPL_QUERY_METHOD(map_t, vector_t)
        INTERFACE_IMPL_METHOD(walk, vector_walk)
        INTERFACE_IMPL_METHOD(put, vector_put)
        INTERFACE_IMPL_METHOD(get, vector_get)
};      

map_t * vector_new()
{
	vector_t * v = slab_calloc(vectors);
	v->map.ops = &vector_t_map_t;
	v->table = vector_table_new(0);

	return com_query(vector_t_map, iid_map_t, v);
}

void vector_test()
{
	vector_init();

	int i = 3;
	map_t * v = vector_new();
	void * p = vector_test;

	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));
	map_putip(v, i, p);
	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));

	i+=61;

	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));
	map_putip(v, i, p);
	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));

	i+=VECTOR_TABLE_ENTRIES;

	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));
	map_putip(v, i, p);
	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));

	i*=VECTOR_TABLE_ENTRIES;

	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));
	map_putip(v, i, p);
	kernel_printk("v[%d] = %p\n", i, map_getip(v, i));

	map_walkip(v, vector_test_walk, 0);
}
