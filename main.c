#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

char* eat_bracket_command(char** src) {
	assert(**src == '[');
	(*src)++;

    const int LINE_LENGTH = 255;
    char* cmd_buffer = malloc(LINE_LENGTH + 1);
    int buf_i = 0;

    char c;
	while ((c = **src)) {
		if (c == ']') break;
        cmd_buffer[buf_i++] = c;
        assert(buf_i < LINE_LENGTH);

		(*src)++;
	}

    cmd_buffer[buf_i] = '\0';
    
    printf("CMDBUF: '%s'\n", cmd_buffer);

	(*src)++;

    return cmd_buffer;
}

char* eat_line(char** src) {
    const int LINE_LENGTH = 127;
    char* line_buf = malloc(LINE_LENGTH + 1);
    int line_i = 0;

    char c;
    while ((c = **src)) {
        char c = **src;
        if (c == '\n') break;

        line_buf[line_i++] = c;
        assert(line_i < LINE_LENGTH);

        (*src)++;
    }

    line_buf[line_i] = '\0';

    return line_buf;
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

void parse_some(char** src);

void process_command(char* cmd_line, char** src) {
    printf("Run: '%s'\n", cmd_line);

    if (strcmp(cmd_line, "iscript") == 0) {
        // Keep looping until we find a thing that closes it
        while (**src) {
            // If we find '\n@' we might be onto something
            if (*((*src)++) != '\n') continue;
            if (**src != '@') continue;
            (*src)++;

            char* line = eat_line(src);
            bool out_we_go = strcmp(line, "endscript") == 0;
            free(line);
            if (out_we_go) break;
        }
    }
}

void parse_some(char** src) {
    char c = **src;
    assert(c);

    if (is_whitespace(c)) {
        (*src)++;
        return;
    }

    //printf("%d: '%c'\n", c, c);
    if (c == '[') {
        char* command = eat_bracket_command(src);
        process_command(command, src);
    } else if (c == '@') {
        (*src)++;
        char* command = eat_line(src);
        process_command(command, src);
    } else if (c == '*') {
        printf("TODO!!\n");
        assert(false);
    } else if (c == ';') {
        // Eat comment
        while(++(*src)) {
            if (**src == '\n') break;
        }
    } else {
        printf("Didn't expect '%c' (%d)\n", c, c);
        assert(false);
    }
}


int main() {
	char* src_str = load_src("cache/scenario/first.ks");
	char* src = src_str;

	printf("FataMoru!! ^-^\n");

	while (*src) {
        parse_some(&src);
	}
}
