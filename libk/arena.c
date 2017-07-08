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

static void arena_mark(void * p);
static slab_type_t arenas[1] = {SLAB_TYPE(sizeof(arena_t), arena_mark, 0)};
static arena_t * arena_create(size_t size)
{
	arena_t * arena = slab_alloc(arenas);

	arena->base = arena->state = vm_kas_get(size);
	arena->seg = vm_segment_anonymous(arena->base, size, SEGMENT_R | SEGMENT_W);
	arena->top  = arena->base + size;
	map_putpp(kas, arena->seg->base, arena->seg);

	return arena;
}

static void arena_mark(void * p)
{
	arena_t * arena = (arena_t *)p;

	slab_gc_mark_range(arena->base, arena->state);
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

static arena_t * free_arenas = 0;

arena_t * arena_get()
{
	arena_t * arena = 0;

	thread_lock(&free_arenas);

	if (free_arenas) {
		arena = free_arenas;
		free_arenas = free_arenas->next;
	} else {
		arena = arena_create(0x400000);
	}

	thread_unlock(&free_arenas);

	return arena;
}

void arena_free(arena_t * arena)
{
	thread_lock(&free_arenas);
	arena->next = free_arenas;
	arena->state = arena->base;
	free_arenas = arena;
	thread_unlock(&free_arenas);
}

void arena_test()
{
	arena_t * arena = arena_get();
	arena_state state = arena_getstate(arena);
	int * p1 = arena_alloc(arena, sizeof(int));
	int * p2 = arena_alloc(arena, sizeof(int));
	*p1 = 0x18767686;
	*p2 = 0xef65da9d;
	arena_setstate(arena, state);
	int * p3 = arena_alloc(arena, sizeof(int));
	int * p4 = arena_alloc(arena, sizeof(int));

	thread_gc();

	assert(*p1 == *p3);
	assert(*p2 == *p4);
}
