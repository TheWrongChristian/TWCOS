#include "vector.h"

#if INTERFACE

#include <stdint.h>

typedef struct vector_s {
	struct map_ops * ops;
	struct vector_table_s * table;
} vector_t;

#endif

#define VECTOR_TABLE_ENTRIES_LOG2 6
#define VECTOR_TABLE_ENTRIES (1<<VECTOR_TABLE_ENTRIES_LOG2)

typedef struct vector_table_s {
	int level;

	intptr_t d[VECTOR_TABLE_ENTRIES];
} vector_table_t;

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
	} else {
		return table->d+i;
	}
}

static void vector_checksize(vector_t * v, map_key i)
{
	/* Extend the table as necessary */
	while(1<<(VECTOR_TABLE_ENTRIES_LOG2*(v->table->level+1))<i) {
		vector_table_t * table = vector_table_new(v->table->level+1);
		table->d[0] = (intptr_t)v->table;
		v->table = table;
	}
}

static map_data vector_put(map_t * m, map_key i, map_data d)
{
	vector_t * v = (vector_t*)m;
	vector_checksize(v, i);
	intptr_t * entry = vector_entry_get(v->table, i, 1);
	intptr_t old = *entry;
	*entry = d;
	return old;
}

static map_data vector_get(map_t * m, map_key i)
{
	vector_t * v = (vector_t*)m;
	intptr_t * entry = vector_entry_get(v->table, i, 0);

	if (entry) {
		return *entry;
	}

	return 0;
}

static void vector_walk_table(vector_t * v, vector_table_t * t, void * arg, int base, walk_func f)
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

static void vector_walk(map_t * m, walk_func f, void * arg )
{
	vector_t * v = (vector_t*)m;
	if (v->table) {
		vector_walk_table(v, v->table, arg, 0, f);
	}
}

static void vector_test_walk(void * ignored, map_key i, void * p)
{
	kernel_printk("v[%d] = %p\n", i, p);
}

map_t * vector_new()
{
	vector_t * v = slab_calloc(vectors);
	static struct map_ops vector_ops = {
                destroy: 0,
                walk: vector_walk,
                put: vector_put,
                get: vector_get,
                get_le: 0,
                optimize: 0,
                remove: 0 /* vector_remove */,
                iterator: 0 /* vector_iterator */
        };

	v->ops = &vector_ops;
	v->table = vector_table_new(0);

	return (map_t*)v;
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
