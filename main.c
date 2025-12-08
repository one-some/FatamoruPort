#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

char* load_src(const char* path) {
	FILE* f = fopen(path, "rb");
	assert(f);

	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* src = malloc(len);
	assert(src);
	fread(src, 1, len, f);
	fclose(f);

	return src;
}

void eat_bracket_command(char** src) {
	assert(**src == '[');
	(*src)++;

	while (**src) {

		printf("Bleh: %c\n", **src);
		if (**src == ']') break;
		(*src)++;
	}

	(*src)++;
}

bool is_whitespace(char c) {
	switch (c) {
		case '\n':
		case ' ':
		case '\t':
			return true;
	}
	return false;
}

int main() {
	char* src_str = load_src("cache/scenario/first.ks");
	char* src = src_str;

	printf("FataMoru!! ^-^\n");

	while (*src) {
		char c = *src;

		if (is_whitespace(c)) {
			(*src)++;
			continue;
		}

		printf("%d: '%c'\n", c, c);

		if (c == '[') {
			eat_bracket_command(&src);
		} else {
			printf("Didn't expect '%c'\n", c);
			assert(false);
		}

	}
}
