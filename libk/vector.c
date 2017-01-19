#include "vector.h"

#if INTERFACE

typedef struct vector_s {
	struct vector_table_s * table;
} vector_t;

#endif

#define VECTOR_TABLE_ENTRIES_LOG2 6
#define VECTOR_TABLE_ENTRIES (1<<VECTOR_TABLE_ENTRIES_LOG2)

typedef struct vector_table_s {
	int level;

	void * d[VECTOR_TABLE_ENTRIES];
} vector_table_t;

static slab_type_t vectors[1];
static slab_type_t tables[1];

void vector_init()
{
	slab_type_create(vectors, sizeof(vector_t), 0, 0);
	slab_type_create(tables, sizeof(vector_table_t), 0, 0);
}

static vector_table_t * vector_table_new(int level)
{
	vector_table_t * table = slab_calloc(tables);

	table->level = level;

	return table;
}

static void ** vector_entry_get(vector_table_t * table, int i)
{
	if (table->level) {
		int shift = VECTOR_TABLE_ENTRIES_LOG2*table->level;
		int index = i>>shift;
		if (0 == table->d[index]) {
			table->d[index] = vector_table_new(table->level-1);
		}

		return vector_entry_get(table->d[index], i&((1<<shift)-1));
	} else {
		return table->d+i;
	}
}

static void vector_checksize(vector_t * v, int i)
{
	/* Extend the table as necessary */
	while(1<<(VECTOR_TABLE_ENTRIES_LOG2*(v->table->level+1))<i) {
		vector_table_t * table = vector_table_new(v->table->level+1);
		table->d[0] = v->table;
		v->table = table;
	}
}

void * vector_put(vector_t * v, int i, void * p)
{
	vector_checksize(v, i);
	void ** entry = vector_entry_get(v->table, i);
	void * old = *entry;
	*entry = p;
	return old;
}

void * vector_get(vector_t * v, int i)
{
	vector_checksize(v, i);
	void ** entry = vector_entry_get(v->table, i);

	return *entry;
}

vector_t * vector_new()
{
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

	kernel_printk("v[%d] = %p\n", i, vector_get(v, i));
	vector_put(v, i, p);
	kernel_printk("v[%d] = %p\n", i, vector_get(v, i));

	i+=VECTOR_TABLE_ENTRIES;

	kernel_printk("v[%d] = %p\n", i, vector_get(v, i));
	vector_put(v, i, p);
	kernel_printk("v[%d] = %p\n", i, vector_get(v, i));
}
