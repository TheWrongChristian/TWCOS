#include "queue.h"

#if INTERFACE
#include <stdint.h>

struct queue_t {
	monitor_t lock[1];
	uintptr_t * qbuf;
	int head;
	int tail;
	int size;
};

#endif

queue_t * queue_new(int size)
{
	queue_t * queue = calloc(1, sizeof(*queue));
	queue->qbuf = calloc(size, sizeof(*queue->qbuf));
	queue->size = size;

	return queue;
}

#define QUEUE_INDEX(queue, i) (i%queue->size)

static int queue_empty_locked(queue_t * queue)
{
	return (queue->head == queue->tail);
}

int queue_empty(queue_t * queue)
{
	int empty = 0;
	MONITOR_AUTOLOCK(queue->lock) {
		empty = queue_empty_locked(queue);
	}

	return empty;
}

static int queue_full_locked(queue_t * queue)
{
	return (queue->head == QUEUE_INDEX(queue, queue->tail+1));
}

int queue_full(queue_t * queue)
{
	int full = 0;
	MONITOR_AUTOLOCK(queue->lock) {
		full = queue_full_locked(queue);
	}

	return full;
}


void queue_put(queue_t * queue, uintptr_t d)
{
	MONITOR_AUTOLOCK(queue->lock) {
		while(queue_full_locked(queue)) {
			monitor_wait(queue->lock);
		}

		queue->qbuf[queue->tail] = d;
		queue->tail = QUEUE_INDEX(queue, queue->tail+1);
		monitor_signal(queue->lock);
	}
}

void queue_putp(queue_t * queue, void * p)
{
	queue_put(queue, (uintptr_t)p);
}

uintptr_t queue_get(queue_t * queue)
{
	uintptr_t d = 0;
	MONITOR_AUTOLOCK(queue->lock) {
		while(queue_empty_locked(queue)) {
			monitor_wait(queue->lock);
		}

		d = queue->qbuf[queue->head];
		queue->qbuf[queue->head] = 0;
		queue->head = QUEUE_INDEX(queue, queue->head+1);
                monitor_signal(queue->lock);
	}

	return d;
}

void * queue_getp(queue_t * queue)
{
	return (void*)queue_get(queue);
}
