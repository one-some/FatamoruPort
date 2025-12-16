#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "rosetta.h"
#include "mem.h"

void* a_malloc(MemArena* arena, size_t size) {
	// printf("[ar] Malloc\n");
    if (!arena->base) {
        arena->size = 0x3FFFFF;
        arena->base = malloc(arena->size);
		arena->last_clean = false;

        assert(arena->base);
    }

    uintptr_t current = (uintptr_t)arena->base + arena->offset;
    uintptr_t padding = 0;

    // Hehe some alignment help
    uintptr_t error = current & (sizeof(void*) - 1);
    if (error != 0) {
        padding = sizeof(void*) - error;
    }

    assert(arena->offset + padding + size <= arena->size);

	arena->last_offset = arena->offset;
	arena->last_clean = true;

    size_t allocated_offset = arena->offset + padding;
    arena->offset += padding + size;

    return (void*)(arena->base + allocated_offset);
}

void a_resize_last_alloc(MemArena* arena, size_t size) {
	// printf("[ar] Resize\n");
	assert(arena->last_clean);

	arena->last_clean = false;
	arena->offset = arena->last_offset;

    uintptr_t current = (uintptr_t)arena->base + arena->offset;
    uintptr_t padding = 0;

    uintptr_t error = current & (sizeof(void*) - 1);
    if (error != 0) {
        padding = sizeof(void*) - error;
    }

    assert(arena->offset + padding + size <= arena->size);
	arena->last_offset = arena->offset;

    size_t allocated_offset = arena->offset + padding;
    arena->offset += padding + size;
}
