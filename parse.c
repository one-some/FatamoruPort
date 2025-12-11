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

CommandNode* command_from_line(char* line) {
	CommandNode* out = malloc(sizeof(CommandNode));
	out->base = (BaseNode) { .type = NODE_COMMAND };
	out->args = v_new();
	out->data_type = CMD_DATA_NONE;
	out->data = NULL;

	Vector parts = slice_command(line);

    for (int i=0; i<parts.length; i++) {
        char* arg_str = v_get(&parts, i);

        CommandArg* arg = malloc(sizeof(CommandArg));
        arg->key = malloc(64);
        arg->value = malloc(64);
        arg->value[0] = '\0';

        char* buf = arg->key;
        int buf_i = 0;

        while (*arg_str) {
            if (*arg_str == '=') {
                buf[buf_i] = '\0';
                buf = arg->value;
                buf_i = 0;

                arg_str++;
                continue;
            }

            buf[buf_i++] = *arg_str;
            assert(buf_i < 64);
            arg_str++;
        }

        buf[buf_i] = '\0';
        strip_quotes(arg->value);
        v_append(&out->args, arg);
    }

    return out;
}

CommandNode* eat_bracket_command(char** src) {
	assert(**src == '[');
	(*src)++;

    const int LINE_LENGTH = 511;
	char* line = malloc(LINE_LENGTH + 1);
    int line_i = 0;

    char c;
	while ((c = **src)) {
		if (c == ']') break;
        line[line_i++] = c;
        assert(line_i < LINE_LENGTH);

		(*src)++;
	}

    line[line_i] = '\0';
	(*src)++;

    CommandNode* out = command_from_line(line);
    free(line);
	return out;
}

CommandNode* eat_at_command(char** src) {
	assert(**src == '@');
	(*src)++;

    const int LINE_LENGTH = 127;
	char* line = malloc(LINE_LENGTH + 1);
    int line_i = 0;

    char c;
    while ((c = **src)) {
        char c = **src;
        if (c == '\n') break;

        line[line_i++] = c;
        assert(line_i < LINE_LENGTH);

        (*src)++;
    }
    line[line_i] = '\0';

    CommandNode* out = command_from_line(line);
    free(line);
    return out;
}

void process_command(CommandNode* command, char** src) {
    CommandArg* arg = v_get(&command->args, 0);
    char* cmd = arg->key;

    if (strcmp(cmd, "iscript") == 0) {
        // Keep looping until we find a thing that closes it
        while (**src) {
            // If we find '\n@' we might be onto something
            if (*((*src)++) != '\n') continue;

            char c = **src;
            if (c != '@' && c != '[') continue;

            CommandNode* node;

            if (c == '@') {
                node = eat_at_command(src);
            } else if (c == '[') {
                node = eat_bracket_command(src);
            }

            assert(node);
            
            CommandArg* n_cmd = v_get(&node->args, 0);
            bool out_we_go = strcmp(n_cmd->key, "endscript") == 0;
            if (out_we_go) break;
        }
    } else if (strcmp(cmd, "if") == 0) {
        // TODO
        while (**src) {
            BaseNode* node = parse_one(src);
            if (!node) continue;
            if (node->type != NODE_COMMAND) continue;
            CommandNode* cmd_node = (CommandNode*)node;

            char* n_cmd = ((CommandArg*)v_get(&cmd_node->args, 0))->key;
            if (strcmp(n_cmd, "endif") == 0) break;
        }
    } else if (strcmp(cmd, "macro") == 0) {
		Vector* nodes = malloc(sizeof(Vector));
		*nodes = v_new();

		command->data_type = CMD_DATA_MACRO;
		command->data = nodes;

        while (**src) {
            BaseNode* node = parse_one(src);
            if (!node) continue;

            print_node(node, "parse-macro");
			v_append(nodes, node);

            if (node->type != NODE_COMMAND) continue;
            CommandNode* cmd_node = (CommandNode*)node;

            char* n_cmd = ((CommandArg*)v_get(&cmd_node->args, 0))->key;
            if (strcmp(n_cmd, "endmacro") == 0) break;
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
        process_command(command, src);
		return (BaseNode*)command;
    } else if (c == '@') {
        CommandNode* command = eat_at_command(src);
        process_command(command, src);
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
		TextNode* out = malloc(sizeof(TextNode));
		out->base = (BaseNode) { .type = NODE_TEXT };
		out->text = malloc(256);

		char c;
		int i = 0;
		while ((c = **src)) {
			
			if (c == '@') break;
			if (c == '[') break;
			if (c == '*') break;
			if (c == ';') break;

			assert(i < 255);
			out->text[i++] = c;

			(*src)++;
		}
		out->text[i] = '\0';

		return (BaseNode*)out;
    }

	return NULL;
}

void print_node(BaseNode* base_node, const char* context) {
    printf("[%s] ", context);

	if (base_node->type == NODE_LABEL) {
		LabelNode* node = (LabelNode*)base_node;
		printf("[lbl] LabelID: '%s'", node->label_id);
		if (node->label_title) printf(" Title: '%s'", node->label_title);
		printf("\n");
	} else if (base_node->type == NODE_COMMAND) {
		CommandNode* node = (CommandNode*)base_node;
		printf("[cmd]");

        for (int i=0; i<node->args.length; i++) {
            CommandArg* arg = v_get(&node->args, i);
            printf(" %s", arg->key);
            if (arg->value && *arg->value) {
                printf(":%s", arg->value);
            }
        }

        printf("\n");
	} else if (base_node->type == NODE_TEXT) {
	    TextNode* node = (TextNode*)base_node;
		printf("[txt] '%s'", node->text);
    }
}

char* get_arg_str(Vector* args, char* key) {
    for (int i=0; i<args->length; i++) {
        CommandArg* arg = v_get(args, i);
        if (strcmp(arg->key, key) == 0) return arg->value;
    }

    return NULL;
}

int get_arg_int(Vector* args, char* key) {
    char* str = get_arg_str(args, key);
    if (!str) return NO_ARG_INT;

    // YOLO
    int out = atoi(str);

    // Whatever man
    assert(out != NO_ARG_INT);

    return out;
}

void strip_quotes(char* str) {
    // hehe not mine. I'm evil and lazy
    size_t len = strlen(str);

    if (len > 1 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

Vector slice_command(char* cmd) {
	Vector parts = v_new();

    const int MAX_BUF_LEN = 127;
	char* active_buffer = NULL;
	int i = 0;

	while (*cmd) {
		if (!active_buffer) {
			active_buffer = malloc(MAX_BUF_LEN + 1);
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
                assert(i < MAX_BUF_LEN);
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

