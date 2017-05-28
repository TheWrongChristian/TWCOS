#include "arena.h"

#if INTERFACE
#include <stddef.h>

typedef struct arena_s
{
	segment_t * seg;
	char * base;
	char * state;
	char * top;

	struct arena_s * next;
} arena_t;

typedef void * arena_state;

#endif

static slab_type_t arenas[1] = {SLAB_TYPE(sizeof(arena_t), 0, 0)};
arena_t * arena_create(size_t size)
{
	arena_t * arena = slab_alloc(arenas);

	arena->base = arena->state = vm_kas_get(size);
	arena->seg = vm_segment_anonymous(arena->base, size, SEGMENT_R | SEGMENT_W);
	arena->top  = arena->base + size;
	map_putpp(kas, arena->base, arena);

	return arena;
}

void * arena_alloc(arena_t * arena, size_t size)
{
	void * p = arena->state;
	size += (sizeof(intptr_t)-1);
	size &= ~(sizeof(intptr_t)-1);
	arena->state += size;

	return p;
}

arena_state arena_getstate(arena_t * arena)
{
	return (arena_state)arena->state;
}

void arena_setstate(arena_t * arena, arena_state state)
{
	check_ptr_bounds(state, arena->base, arena->top, "Arena state pointer out of arena bounds");
	arena->state = (char*)state;
}

static arena_t * free = 0;

arena_t * arena_get()
{
	arena_t * arena = 0;

	thread_lock(&free);

	if (free) {
		arena = free;
		free = free->next;
	} else {
		arena = arena_create(0x400000);
	}

	thread_unlock(&free);

	return arena;
}

void arena_free(arena_t * arena)
{
	thread_lock(&free);
	arena->next = free;
	free = arena;
	thread_unlock(&free);
}
