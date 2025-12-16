#pragma once
#include <stddef.h>
#include <stdint.h>

// TODO: Flystrings

typedef struct {
    unsigned char* base;
    uintptr_t offset;
    size_t size;
} MemArena;

void* a_malloc(MemArena* arena, size_t size);
