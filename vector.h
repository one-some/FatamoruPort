#pragma once
#include <stddef.h>

typedef struct {
	size_t length;
	size_t mem_length;
	void** content;
} Vector;

Vector v_new();
void* v_get(Vector* v, size_t idx);
void* v_pop(Vector* v, size_t idx);
size_t v_append(Vector* v, void* value);
void v_deepfree(Vector* v);

