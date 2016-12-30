#include "iterator.h"

#if INTERFACE

typedef struct {
	void * (*next)(struct iterator * this);
	void * (*remove)(struct iterator * this);
	void (*destroy)(struct iterator * this);
} iterator_ops;

typedef struct iterator{
	iterator_ops * ops;

	/* container specific data */
	void * data;
} iterator_t;

#endif

void * iterator_next(iterator_t * iterator)
{
	return iterator->ops->next(iterator);
}

void * iterator_remove_current(iterator_t * iterator)
{
	return iterator->ops->remove(iterator);
}

void iterator_destroy(iterator_t * iterator)
{
	iterator->ops->destroy(iterator);
}
