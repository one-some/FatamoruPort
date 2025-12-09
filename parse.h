#pragma once

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


bool is_whitespace(char c);
char* load_src(const char* path);
LabelNode* eat_label(char** src);
CommandNode* eat_bracket_command(char** src);
CommandNode* eat_at_command(char** src);
void process_command(char* cmd_line, char** src);
BaseNode* parse_one(char** src);
void print_node(BaseNode* base_node);
