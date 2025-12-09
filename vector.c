#include "vector.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

Vector v_new() {
	return (Vector) {
		0,
		0,
		NULL
	};
}

void* v_get(Vector* v, size_t idx) {
	assert(v->length > idx);
	return v->content[idx];
}

void* v_pop(Vector* v, size_t idx) {
	assert(v->length > idx);
	void* val = v->content[idx];

	for (int i=idx; i < v->length - 1; i++) {
		v->content[i] = v->content[i + 1];
	}

	v->length--;

	return val;
}

size_t v_append(Vector* v, void* value) {
	if (v->mem_length < v->length + 1) {
		size_t new_mem_length = (v->length + 1) * 2;
		void** new_content = malloc(sizeof(void*) * new_mem_length);

		if (v->content) {
			memcpy(
				new_content,
				v->content,
				sizeof(void*) * v->mem_length
			);
			free(v->content);
		}

		v->content = new_content;
		v->mem_length = new_mem_length;
	}

	v->content[v->length] = value;
	return v->length++;
}

void v_deepfree(Vector* v) {
	for (int i = 0; i < v->length; i++) {
		free(v_get(v, i));
	}
}

