#pragma once

#include <stdio.h>
#include <stdlib.h>

void* m_malloc(size_t size) {
	static size_t allocated = 0;

	allocated += size;

	printf("Malloc: %zu\n", size);

	return malloc(size);
}

#pragma GCC poison malloc
