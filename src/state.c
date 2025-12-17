#include "state.h"
#include "parse.h"
#include <stdlib.h>
#include <string.h>

void jump_to_point(FataState* state, char* path, char* label_target) {
	state->stopped = false;
	state->stopped_until_click = false;
    state->node_idx = 0;
    state->target_nodes = &state->nodes;

    printf(
        "[load] Path: '%s', LabelTarget: '%s'\n",
        path ? path : "(empty)",
        label_target ? label_target : "(empty)"
    );

    bool same_path = !path || (state->script_path && strcmp(state->script_path, path) == 0);

    if (path && !same_path) {
        printf("[load] Loading from scratch!\n");
		state->script_path = path;

        state->nodes = v_new();

        // WARNING: When (if) we free later, the ptr will be gone! Save it!!
        char* src_origin = load_src(path);
        char* src = src_origin;

        while (*src) {
            BaseNode* node = parse_one(&state->static_arena, &src);
            if (!node) continue;

            v_append(&state->nodes, node);
        }

        free(src_origin);
	}

	for (int i=0; i<state->nodes.length;i++) {
		BaseNode* node = v_get(&state->nodes, i);

		if (node->type == NODE_COMMAND) {
			CommandNode* cmd = (CommandNode*)node;

			if (cmd->data_type == CMD_DATA_MACRO && !same_path) {
                char* name = get_arg_str(&cmd->args, "name");
                assert(name);

                Macro* macro = NULL;

                for (int j=0; j<state->macros.length; j++) {
                    Macro* macro_candidate = v_get(&state->macros, j);
                    if (strcmp(name, macro_candidate->name) != 0) continue;

                    macro = macro_candidate;
                    break;
                }

                if (macro) {
                    // Do not make duplicate macros!
                    continue;
                }

                macro = malloc(sizeof(Macro));
                macro->name = name;
                macro->children = *(Vector*)cmd->data;

                v_append(&state->macros, macro);
			}
		}

		if (label_target && node->type == NODE_LABEL) {

			// Set node_idx to this label right off the bat
			LabelNode* label = (LabelNode*)node;

			if (strcmp(label->label_id, label_target) != 0) continue;

			state->node_idx = i;
			label_target = NULL;
		}
	}

	if (label_target) {
		printf("[err] Couldn't find label %s\n", label_target);
		assert(!label_target);
	}
}

void push_to_callstack(FataState* state) {
    ScriptLocation* where = malloc(sizeof(ScriptLocation));
    where->script_path = state->script_path;
    where->node_idx = state->node_idx;
    where->target_nodes = state->target_nodes;
    v_append(&state->call_stack, where);
}

void return_from_callstack(FataState* state) {
    assert(state->call_stack.length);

    ScriptLocation* where = v_pop(&state->call_stack, state->call_stack.length - 1);
    assert(where);

    jump_to_point(state, where->script_path, NULL);
    state->node_idx = where->node_idx;

    // This is gonna bite me in the butt later
    state->target_nodes = where->target_nodes;
}

