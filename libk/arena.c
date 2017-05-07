#include "arena.h"

#if INTERFACE
#include <stddef.h>

typedef struct arena_s
{
	segment_t * seg;
	char * base;
	char * next;
	char * top;
} arena_t;

#endif

static slab_type_t arenas[1] = {SLAB_TYPE(sizeof(arena_t), 0, 0)};
arena_t * arena_new(size_t size)
{
	arena_t * arena = slab_alloc(arenas);

	arena->base = arena->next = vm_kas_get(size);
	arena->seg = vm_segment_anonymous(arena->base, size, SEGMENT_R | SEGMENT_W);
	arena->top  = arena->base + size;
	map_putpp(kas, arena->base, arena);

	return arena;
}

void * arena_alloc(arena_t * arena, size_t size)
{
	void * p = arena->next;
	size += (sizeof(intptr_t)-1);
	size &= ~(sizeof(intptr_t)-1);
	arena->next += size;

	return p;
}
