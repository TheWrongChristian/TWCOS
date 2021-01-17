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

#define ARENA_CONCAT(a, b) a ## b
#define ARENA_VAR(line) AUTOLOCK_CONCAT(s,line)
#define ARENA_AUTOSTATE(arena) arena_state ARENA_VAR(__LINE__) = 0; while((ARENA_VAR(__LINE__)=arena_autostate(arena, ARENA_VAR(__LINE__) )))


#endif

static void arena_mark(void * p);
static void arena_finalize(void * p);
static slab_type_t arenas[1] = {SLAB_TYPE(sizeof(arena_t), arena_mark, arena_finalize)};
static arena_t * arena_create(size_t size)
{
	arena_t * arena = slab_alloc(arenas);

	arena->base = arena->state = vm_kas_get_aligned(size, ARCH_PAGE_SIZE);
	arena->seg = vm_segment_anonymous(arena->base, size, SEGMENT_R | SEGMENT_W);
	arena->top  = arena->base + size;
	vm_kas_add(arena->seg);

	return arena;
}

static void arena_mark(void * p)
{
	arena_t * arena = (arena_t *)p;

	slab_gc_mark(arena->seg);
	slab_gc_mark_range(arena->base, arena->state);
}

arena_state arena_autostate(arena_t * arena, arena_state state)
{
	if (0 == arena) {
		arena = arena_thread_get();
	}

	if (state) {
		arena_setstate(arena, state);
		return 0;
	} else {
		return arena_getstate(arena);
	}
}

void * arena_alloc(arena_t * arena, size_t size)
{
	if (0 == arena) {
		arena = arena_thread_get();
	}

	void * p = arena->state;
	size += (sizeof(intptr_t)-1);
	size &= ~(sizeof(intptr_t)-1);
	arena->state += size;

	return p;
}

void * arena_calloc(arena_t * arena, size_t size)
{
	void * p = arena_alloc(arena, size);
	return memset(p, 0, size);
}

void * arena_palloc(arena_t * arena, int pages)
{
	if (0 == arena) {
		arena = arena_thread_get();
	}

	/* Round up to next page boundary */
	void * p = arena->state = ARCH_PAGE_ALIGN(arena->state+ARCH_PAGE_SIZE-1);

	/* Advance past the pages we want */
	arena->state += pages * ARCH_PAGE_SIZE;

	return p;
}

void * arena_pmap(arena_t * arena, vmpage_t * vmpage)
{
	void * p = arena_palloc(arena, 1);

	/* Map the given page */
	vm_page_replace(p, vmpage);

	return p;
}

arena_state arena_getstate(arena_t * arena)
{
	if (0 == arena) {
		arena = arena_thread_get();
	}
	return (arena_state)arena->state;
}

void arena_setstate(arena_t * arena, arena_state state)
{
	if (0 == arena) {
		arena = arena_thread_get();
	}
	check_ptr_bounds(state, arena->base, arena->top, "Arena state pointer out of arena bounds");
	arena->state = (char*)state;
}

static arena_t * free_arenas = 0;
static mutex_t arena_lock[1] = {0};

arena_t * arena_get()
{
	arena_t * arena = 0;

	MUTEX_AUTOLOCK(arena_lock) {
		if (free_arenas) {
			arena = free_arenas;
			free_arenas = free_arenas->next;
		} else {
			arena = arena_create(0x400000);
		}
	}

	return arena;
}

static void arena_finalize(void * p)
{
	arena_free((arena_t *)p);
}

void arena_free(arena_t * arena)
{
	MUTEX_AUTOLOCK(arena_lock) {
		/* Reset the memory used by the arena */
		arena->state = arena->base;
		vmobject_release(arena->seg->dirty);

		/* Chain into available cached arenas */
		arena->next = free_arenas;
		free_arenas = arena;
	}
}

static int arena_key = 0;
arena_t * arena_thread_get()
{
	/* Check we have a valid TLS key, init if not */
	if (0 == arena_key) {
		MUTEX_AUTOLOCK(arena_lock) {
			if (0 == arena_key) {
				arena_key = tls_get_key();
			}
		}
	}

	/* Get the TLS arena, and get a new one if there is none */
	arena_t * arena = tls_get(arena_key);
	if (0 == arena) {
		arena = arena_get();
		tls_set(arena_key, arena);
	}

	return arena;
}

void arena_thread_free()
{
	/* Check we have a valid TLS key, init if not */
	MUTEX_AUTOLOCK(arena_lock) {
		if (0 == arena_key) {
			arena_key = tls_get_key();
		}
	}

	/* Get the TLS arena, and get a new one if there is none */
	arena_t * arena = tls_get(arena_key);
	if (arena) {
		arena_free(arena);
	}
}

void * tmalloc(size_t size)
{
	arena_t * arena = arena_thread_get();

	return arena_alloc(arena, size);
}

void * tcalloc(size_t nmemb, size_t size)
{
	arena_t * arena = arena_thread_get();

	return arena_calloc(arena, nmemb*size);
}

char * tstrndup(const char * s, size_t maxlen)
{
	char * ret = tmalloc(maxlen+1);
	ret[maxlen] = 0;
	return strncpy(ret, s, maxlen);
}

char * tstrdup(const char * s)
{
	int len = strlen(s);
	char * ret = tmalloc(len+1);

	ret[len] = 0;
        return memcpy(ret, s, len);
}

void arena_test()
{
	arena_t * arena = arena_thread_get();
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
