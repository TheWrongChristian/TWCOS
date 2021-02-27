#include "fifo.h"

#if INTERFACE

#include <stdint.h>

struct fifo_t
{
	int size;
	int head;
	int tail;
	intptr_t buf[1];
}

#endif

exception_def FifoException = { "FifoException", &Exception };
exception_def FifoEmptyException = { "FifoEmptyException", &FifoException };
exception_def FifoFullException = { "FifoFullException", &FifoException };

fifo_t * fifo_new(int size)
{
	fifo_t * fifo = calloc(1, sizeof(*fifo) + sizeof(fifo->buf[0]) * size);
	fifo->size = size;

	return fifo;
}

#define FIFO_PTR(fifo, ptr) ((ptr == fifo->size) ? 0 : ptr)

int fifo_full(fifo_t * fifo)
{
	return FIFO_PTR(fifo, fifo->head+1) == FIFO_PTR(fifo, fifo->tail);
}

int fifo_empty(fifo_t * fifo)
{
	return FIFO_PTR(fifo, fifo->head) == FIFO_PTR(fifo, fifo->tail);
}

intptr_t fifo_get(fifo_t * fifo)
{
	if (fifo_empty(fifo)) {
		KTHROW(FifoEmptyException, "FIFO empty");
	}

	const intptr_t v = fifo->buf[fifo->tail];
	fifo->tail = FIFO_PTR(fifo, fifo->tail+1);

	return v;
}

void fifo_put(fifo_t * fifo, intptr_t v)
{
	if (fifo_full(fifo)) {
		KTHROW(FifoFullException, "FIFO full");
	}

	fifo->buf[fifo->head] = v;
	fifo->head = FIFO_PTR(fifo, fifo->head+1);
}
