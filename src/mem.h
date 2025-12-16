#pragma once
#include <stddef.h>
#include <stdint.h>

// TODO: Flystrings

typedef struct {
    unsigned char* base;
    uintptr_t offset;
    size_t size;

    uintptr_t last_offset;
	bool last_clean;
} MemArena;

void* a_malloc(MemArena* arena, size_t size);
void a_resize_last_alloc(MemArena* arena, size_t size);
