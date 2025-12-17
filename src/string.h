#include <stdarg.h>

typedef struct {
	char* base;
	size_t capacity;
	size_t length;
} StringBuilder;

StringBuilder sb_make(size_t capacity) {
	return (StringBuilder) {
		.base = malloc(capacity + 1),
		.capacity = capacity
	};
}

void sb_free(StringBuilder sb) {
	assert(sb.base);
	free(sb.base);
}

void sb_add(StringBuilder* sb, char* string) {
	assert(sb->length + strlen(string) < sb->capacity);

	for (int i=0; string[i]; i++) {
		sb->base[sb->length++] = string[i];
	}

	sb->base[sb->length] = '\0';
}

void sb_addf(StringBuilder* sb, char* format, ...) {
	va_list args;
	va_start(args, format);

	char buf[256];
	vsnprintf(buf, sizeof(buf), format, args);
	sb_add(sb, buf);

	va_end(args);
}
