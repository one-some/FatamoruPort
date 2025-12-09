#pragma once
#include "vector.h"

#define NO_ARG_INT (1 << 31)

typedef enum {
	NODE_COMMAND,
	NODE_LABEL,
} NodeType;

typedef struct {
	NodeType type;
} BaseNode;

typedef struct {
	BaseNode base;
    Vector args;
} CommandNode;

typedef struct {
	BaseNode base;
	char* label_id;
	char* label_title;
} LabelNode;

typedef struct {
    char* key;
    char* value;
} CommandArg;

bool is_whitespace(char c);
char* load_src(const char* path);
LabelNode* eat_label(char** src);
CommandNode* eat_bracket_command(char** src);
CommandNode* eat_at_command(char** src);
void process_command(CommandNode* command, char** src);
BaseNode* parse_one(char** src);
void print_node(BaseNode* base_node);
Vector slice_command(char* cmd);
void strip_quotes(char* str);
char* get_arg_str(Vector* args, char* key);
int get_arg_int(Vector* args, char* key);
