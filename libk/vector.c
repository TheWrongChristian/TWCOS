#include "vector.h"

#if INTERFACE

#include <stdint.h>

typedef struct vector_s {
	struct vector_table_s * table;
} vector_t;

#endif

#define VECTOR_TABLE_ENTRIES_LOG2 6
#define VECTOR_TABLE_ENTRIES (1<<VECTOR_TABLE_ENTRIES_LOG2)

typedef struct vector_table_s {
	int level;

	intptr_t d[VECTOR_TABLE_ENTRIES];
} vector_table_t;

static slab_type_t vectors[1];
static slab_type_t tables[1];

void vector_init()
{
	INIT_ONCE();

	slab_type_create(vectors, sizeof(vector_t), 0, 0);
	slab_type_create(tables, sizeof(vector_table_t), 0, 0);
}

static vector_table_t * vector_table_new(int level)
{
	vector_table_t * table = slab_calloc(tables);

	table->level = level;

	return table;
}

static intptr_t * vector_entry_get(vector_table_t * table, int i, int create)
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

static void vector_checksize(vector_t * v, int i)
{
	/* Extend the table as necessary */
	while(1<<(VECTOR_TABLE_ENTRIES_LOG2*(v->table->level+1))<i) {
		vector_table_t * table = vector_table_new(v->table->level+1);
		table->d[0] = (intptr_t)v->table;
		v->table = table;
	}
}

intptr_t vector_put(vector_t * v, int i, intptr_t d)
{
	vector_checksize(v, i);
	intptr_t * entry = vector_entry_get(v->table, i, 1);
	intptr_t old = *entry;
	*entry = d;
	return old;
}

void * vector_putp(vector_t * v, int i, void * p)
{
	return (void*)vector_put(v, i, (intptr_t)p);
}

intptr_t vector_get(vector_t * v, int i)
{
	intptr_t * entry = vector_entry_get(v->table, i, 0);

	if (entry) {
		return *entry;
	}

	return 0;
}

void * vector_getp(vector_t * v, int i)
{
	return (void*)vector_get(v, i);
}

vector_t * vector_new()
{
	vector_init();
	vector_t * v = slab_calloc(vectors);

	v->table = vector_table_new(0);

	return v;
}

void vector_test()
{
	vector_init();

	int i = 3;
	vector_t * v = vector_new();
	void * p = vector_test;

	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));
	vector_putp(v, i, p);
	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));

	i+=VECTOR_TABLE_ENTRIES;

	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));
	vector_putp(v, i, p);
	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));

	i*=VECTOR_TABLE_ENTRIES;

	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));
	vector_putp(v, i, p);
	kernel_printk("v[%d] = %p\n", i, vector_getp(v, i));
}
