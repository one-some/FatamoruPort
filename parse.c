// (C)laire 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "parse.h"

bool is_whitespace(char c) {
	switch (c) {
		case '\n':
		case ' ':
		case '\t':
			return true;
	}
	return false;
}

char* load_src(const char* path) {
	FILE* f = fopen(path, "rb");
	assert(f);

	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* src = malloc(len + 1);
	assert(src);
	fread(src, 1, len, f);
	fclose(f);

	src[len] = '\0';

	return src;
}

LabelNode* eat_label(char** src) {
	assert(**src == '*');
	(*src)++;

	const int LAB_PART_LEN = 63;

	LabelNode* out = malloc(sizeof(LabelNode));
	out->base = (BaseNode) { .type = NODE_LABEL };
	out->label_id = malloc(LAB_PART_LEN + 1);
	out->label_title = NULL;

	int i = 0;
	char c;

	char* target = out->label_id;

	while ((c = **src)) {
		if (c == '\n') break;
		if (c == '|') {
			target[i] = '\0';

			out->label_title = malloc(LAB_PART_LEN + 1);
			target = out->label_title;
			i = 0;
			(*src)++;
			continue;
		}

		target[i++] = c;
		assert(i < LAB_PART_LEN);

		(*src)++;
	}
	target[i] = '\0';

	return out;
}

CommandNode* eat_bracket_command(char** src) {
	assert(**src == '[');
	(*src)++;

    const int LINE_LENGTH = 255;

	CommandNode* out = malloc(sizeof(CommandNode));
	out->base = (BaseNode) { .type = NODE_COMMAND };
	out->cmd = malloc(LINE_LENGTH + 1);
    int buf_i = 0;

    char c;
	while ((c = **src)) {
		if (c == ']') break;
        out->cmd[buf_i++] = c;
        assert(buf_i < LINE_LENGTH);

		(*src)++;
	}

    out->cmd[buf_i] = '\0';
	(*src)++;

	return out;
}

CommandNode* eat_at_command(char** src) {
	assert(**src == '@');
	(*src)++;

    const int LINE_LENGTH = 127;

	CommandNode* out = malloc(sizeof(CommandNode));
	out->base = (BaseNode) { .type = NODE_COMMAND };
	out->cmd = malloc(LINE_LENGTH + 1);
    int line_i = 0;

    char c;
    while ((c = **src)) {
        char c = **src;
        if (c == '\n') break;

        out->cmd[line_i++] = c;
        assert(line_i < LINE_LENGTH);

        (*src)++;
    }

    out->cmd[line_i] = '\0';

    return out;
}

void process_command(char* cmd_line, char** src) {
    if (strcmp(cmd_line, "iscript") == 0) {
        // Keep looping until we find a thing that closes it
        while (**src) {
            // If we find '\n@' we might be onto something
            if (*((*src)++) != '\n') continue;
            if (**src != '@') continue;

            CommandNode* node = eat_at_command(src);
            bool out_we_go = strcmp(node->cmd, "endscript") == 0;
            if (out_we_go) break;
        }
    }
}

BaseNode* parse_one(char** src) {
	while (**src && is_whitespace(**src)) {
        (*src)++;
	}

    char c = **src;
	if (!c) return NULL;

    if (c == '[') {
        CommandNode* command = eat_bracket_command(src);
        process_command(command->cmd, src);
		return (BaseNode*)command;
    } else if (c == '@') {
        CommandNode* command = eat_at_command(src);
        process_command(command->cmd, src);
		return (BaseNode*)command;
    } else if (c == '*') {
		return (BaseNode*)eat_label(src);
    } else if (c == ';') {
        // Eat comment
        while(++(*src)) {
            if (**src == '\n') break;
        }
		return NULL;
    } else {
        printf("Didn't expect '%c' (%d)\n", c, c);
        assert(false);
    }

	return NULL;
}

void print_node(BaseNode* base_node) {
	if (base_node->type == NODE_LABEL) {
		LabelNode* node = (LabelNode*)base_node;
		printf("[label] ID: '%s'", node->label_id);
		if (node->label_title) printf(" Title: '%s'", node->label_title);
		printf("\n");
	} else if (base_node->type == NODE_COMMAND) {
		CommandNode* node = (CommandNode*)base_node;
		printf("[command] '%s'\n", node->cmd);
	}
}

