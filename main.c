// TODO: Arena allocater
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vector.h"

typedef enum {
	NODE_COMMAND,
	NODE_LABEL,
} NodeType;

typedef struct {
	NodeType type;
} BaseNode;

typedef struct {
	BaseNode base;
	char* cmd;
} CommandNode;

typedef struct {
	BaseNode base;
	char* label_id;
	char* label_title;
} LabelNode;

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

bool is_whitespace(char c) {
	switch (c) {
		case '\n':
		case ' ':
		case '\t':
			return true;
	}
	return false;
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

Vector slice_command(char* cmd) {
	Vector parts = v_new();

	char* active_buffer = NULL;
	int i = 0;

	while (*cmd) {
		if (!active_buffer) {
			active_buffer = malloc(64);
			i = 0;
		}

		if (*cmd == ' ') {
			while (*cmd == ' ') cmd++;
			if (!*cmd) break;

			bool next_eq = (*cmd == '=');
            bool prev_eq = (i > 0 && active_buffer[i-1] == '=');

			if (!next_eq && !prev_eq) {
				active_buffer[i] = '\0';
				v_append(&parts, active_buffer);
				active_buffer = NULL;

				continue;
			}

			if (next_eq) {
				active_buffer[i++] = '=';
				cmd++;
			}

			continue;
		}

		if (!active_buffer) continue;
		active_buffer[i++] = *cmd;
		cmd++;
	}

	active_buffer[i] = '\0';
	v_append(&parts, active_buffer);

	return parts;
}

void run_command(CommandNode* command) {
	Vector parts = slice_command(command->cmd);

	printf("Parts:\n");
	for (int i=0;i<parts.length; i++) {
		printf("'%s'\n", v_get(&parts, i));
	}

	char* cmd = v_pop(&parts, 0);

	if (strcmp("wait", cmd) == 0) {
	} else {
		// Ignore!
		return;
	}
}

int main() {
	char* src_str = load_src("cache/scenario/first.ks");
	char* src = src_str;

	BaseNode* nodes[1000];
	int ni = 0;

	printf("FataMoru!! ^-^\n");

	while (*src) {
        BaseNode* node = parse_one(&src);
		if (!node) continue;

		nodes[ni++] = node;
		assert(ni < 1000);
	}
	nodes[ni] = NULL;

	for (int i=0; nodes[i]; i++) {
		print_node(nodes[i]);

		if (nodes[i]->type == NODE_COMMAND) {
			run_command((CommandNode*)nodes[i]);
		}
	}
}
