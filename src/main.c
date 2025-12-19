// TODO: Arena allocater
#include "fs.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "vector.h"
#include "parse.h"
#include "visual.h"
#include "ui.h"
#include "state.h"
#include "rosetta.h"

const char* useful_scripts[] = {
	"macro.ks",
	"macros_sprites.ks",
	NULL
};


bool evaluate_expression(FataState* state, char* expression) {
	printf("Evaluate: '%s'\n", expression);

	if (strcmp(expression, "sf.fataend==1") == 0) return false;
	if (strcmp(expression, "sf.fataend!=1") == 0) return true;

	// YOLO
	return false;
}

bool run_command(CommandNode* command, FataState* state) {
    // returns true if showstopper
    Vector* args = &command->args;
	char* cmd = ((CommandArg*)v_get(args, 0))->key;

	bool done = r_command_hook(state, cmd, args);
	if (done) return false;

	if (strcmp("endmacro", cmd) == 0) {
        return_from_callstack(state);
        return true;

    } else if (strcmp("wait", cmd) == 0) {
        int time_ms = get_arg_int(args, "time");
        assert(time_ms != NO_ARG_INT);

		bool can_skip = true;
        char* can_skip_str = get_arg_str(args, "canskip");
		if (can_skip_str) {
			can_skip = strcmp("false", can_skip_str) != 0;
		}

		state->can_skip_wait = can_skip;
        state->wait_timer_ms = (float)time_ms;
        return true;
    } else if (strcmp("image", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        assert(storage);

        char* path = find_image(storage);
        assert(path);

        char* page_str = get_arg_str(args, "page");
        assert(page_str);

        char* layer_str = get_arg_str(args, "layer");
        assert(layer_str);

		printf("Lol\n");
        VisualLayer* layer = get_layer(state, layer_str, page_str);
		printf("Ok no\n");

        if (layer->texture.valid) {
			r_unload_texture(layer->texture);
        }

        layer->texture = r_load_texture(path);

        int top_px = get_arg_int(args, "top");
		if (top_px != NO_ARG_INT) {
			layer->texture_offset.y = top_px;
		}

        int left_px = get_arg_int(args, "left");
		if (left_px != NO_ARG_INT) {
			layer->texture_offset.x = left_px;
		}

    } else if (strcmp("jump", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
		char* target = get_arg_str(args, "target");
		jump_to_point(state, storage, target);

        return true;
    } else if (strcmp("playbgm", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        assert(storage);

        char* path = find_bgm(storage);
        assert(path);

        if (state->audio.bgm.valid) {
			r_stop_sound(state->audio.bgm);
			r_unload_sound(state->audio.bgm);
        }

        state->audio.bgm = r_load_sound(path);
        r_play_sound(state->audio.bgm);
    } else if (strcmp("playse", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        assert(storage);

        char* path = find_sfx(storage);
        assert(path);

        RSound track = r_load_sound(path);
        r_play_sound(track);
    } else if (strcmp("trans", cmd) == 0) {
        char* method = get_arg_str(args, "method");
        assert(method);

        int time_ms = get_arg_int(args, "time");
        assert(time_ms != NO_ARG_INT);
        state->transition_max_ms = (float)time_ms;
        state->transition_remaining_ms = state->transition_max_ms;
    } else if (strcmp("wt", cmd) == 0) {
        char* can_skip_str = get_arg_str(args, "canskip");

		bool can_skip = true;
		if (can_skip_str) {
			can_skip = strcmp("false", can_skip_str) != 0;
		}

		state->can_skip_transition = can_skip;
        state->wait_for_transition = true;
        return true;
    } else if (strcmp("s", cmd) == 0) {
        // STOP!
        state->stopped = true;
        return true;
    } else if (strcmp("locate", cmd) == 0) {
		// Hey Claire, if you're debugging this because things are offset,
		// take a look at system/Config.tjs again. Remember the margins!
		// XXOXOO, Claire
		VisualLayer* layer = state->active_screen->active_layer;

        int x = get_arg_int(args, "x");
        if (x != NO_ARG_INT) {
			layer->pointer_pos.x = x + layer->margins.left;
		}

        int y = get_arg_int(args, "y");
        if (y != NO_ARG_INT) {
			layer->pointer_pos.y = y + layer->margins.top;
		}

        printf("[locate] X:%d, %s\n", layer->pointer_pos.x, state->active_screen->name);

    } else if (strcmp("current", cmd) == 0) {
		char* layer = get_arg_str(args, "layer");
		assert(layer);

        state->active_screen->active_layer = get_layer(state, layer, NULL);
    } else if (strcmp("button", cmd) == 0) {
		// button graphic:選択ライン hint:"Exit the game." target:*end enterse:button
		char* bg_storage = get_arg_str(args, "graphic");
		assert(bg_storage);
        char* path = find_image(bg_storage);
        assert(path);

        int flags = 0;

        // TODO: has_arg?
		if (get_arg_str(args, "native")) {
            flags |= BUTTON_NATIVE;
        }

        char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

		ButtonObject* button = create_button(
			state, 
			r_load_texture(path),
			get_arg_str(args, "storage"),
			target,
			flags
		);

        // Blehhh!!
        button->enter_se.valid = false;
        char* enter_se_storage = get_arg_str(args, "enterse");
        if (enter_se_storage) {
            button->enter_se = r_load_sound(find_sfx(enter_se_storage));
        }
    } else if (strcmp("close", cmd) == 0) {
		printf("[end] Goodbye from The House in Fata Morgana...\n");
		exit(0);
		return true;
    } else if (strcmp("p", cmd) == 0) {
		state->stopped_until_click = true;
		return true;
    } else if (strcmp("call", cmd) == 0) {
		char* storage = get_arg_str(args, "storage");
		assert(storage);

		bool cool = false;
		for (int i=0; useful_scripts[i]; i++) {
			if (strcmp(useful_scripts[i], storage) != 0) continue;
			cool = true;
			break;
		}

		if (!cool) {
			printf("[call] Ignoring call to '%s'\n", storage);
			return false;
		}

		char* target = get_arg_str(args, "target");
        push_to_callstack(state);

        jump_to_point(state, storage, target);
		return true;
    } else if (strcmp("return", cmd) == 0) {
        return_from_callstack(state);
		return true;
    } else if (strcmp("cm", cmd) == 0) {
		VisualLayer* layers[] = {
			&state->active_screen->fore.message_layer_zero,
			&state->active_screen->fore.message_layer_one,
			&state->active_screen->back.message_layer_zero,
			&state->active_screen->back.message_layer_one,
			NULL
		};

		for (int i=0; layers[i]; i++) {
			VisualLayer* layer = layers[i];
			v_clear(&layer->children);

			layer->pointer_pos.x = layer->margins.left;
			layer->pointer_pos.y = layer->margins.top;

			printf("Resetting to (%d, %d)\n", layer->margins.left, layer->margins.top);
		}
    } else if (strcmp("font", cmd) == 0) {
		RFont* font = &state->active_screen->active_layer->font;

		char* size_str = get_arg_str(args, "size");
		if (size_str) {
			if (size_str[0] >= '0' && size_str[0] <= '9') {
				font->size = (int)strtol(size_str, NULL, 10);
			}

			exit(0);
			// TODO: 'default'
		}

		char* color = get_arg_str(args, "color");
		if (color) {
			printf("Color: '%s'\n", color);
			assert(color[0] == '0');
			assert(color[1] == 'x');
			long rgb = strtol(color + 2, NULL, 16);

			font->color.r = ((rgb & 0xFF0000) >> 16);
			font->color.g = ((rgb & 0x00FF00) >> 8);
			font->color.b = ((rgb & 0x0000FF));
		}
	} else if (strcmp("backlay", cmd) == 0) {
		copy_page(&state->active_screen->back, &state->active_screen->fore);
	} else if (strcmp("position", cmd) == 0) {
		VisualLayer* layer = get_layer(
			state,
			get_arg_str(args, "layer"),
			get_arg_str(args, "page")
		);

		// TODO: Default to active_layer?
		assert(layer);

        int margin_l = get_arg_int(args, "marginl");
        if (margin_l != NO_ARG_INT) layer->margins.left = margin_l;

        int margin_t = get_arg_int(args, "margint");
        if (margin_t != NO_ARG_INT) layer->margins.top = margin_t;

        int margin_r = get_arg_int(args, "marginr");
        if (margin_r != NO_ARG_INT) layer->margins.right = margin_r;

        int margin_b = get_arg_int(args, "marginb");
        if (margin_b != NO_ARG_INT) layer->margins.bottom = margin_b;

		char* frame_storage = get_arg_str(args, "frame");
		if (frame_storage) {
			char* path = find_image(frame_storage);
			assert(path);

			layer->texture = r_load_texture(path);
		}
    } else if (strcmp("title", cmd) == 0) {
        char* name = get_arg_str(args, "name");
		assert(name);
		r_set_window_title(name);
    } else if (strcmp("r", cmd) == 0) {
		VisualLayer* layer = state->active_screen->active_layer;
		layer->pointer_pos.x = layer->margins.left;

		// TODO: Spacing...
		layer->pointer_pos.y += layer->font.size + layer->font.spacing;
	} else if (strcmp("if", cmd) == 0) {
		assert(command->data_type == CMD_DATA_IF);
		Vector* clauses = (Vector*)command->data;
		
		printf("Running if statement...\n");

		IfClause* target_clause = NULL;
		for (int i=0; i<clauses->length; i++) {
			IfClause* clause = v_get(clauses, i);

			printf("Clause condition: '%s'\n", clause->condition);

			// If there is no condition, this is an else block. Run it
			if (
				clause->condition &&
				!evaluate_expression(state, clause->condition)
			) {
				printf("Noway\n");
				continue;
			}
			printf("Setting\n");

			target_clause = clause;
			break;
		}
		// assert(target_clause);
		if (!target_clause) return false;

		for (int j=0; j<target_clause->children.length; j++) {
			printf(" - ");
			BaseNode* child = v_get(&target_clause->children, j);
			print_node(child, "exec-if");
		}

		push_to_callstack(state);
		state->target_nodes = &target_clause->children;
		state->node_idx = 0;
		return true;
	}





    // Ported macros
    if (strcmp("ctrlSpeakingPerson", cmd) == 0) {
        char* speaker = get_arg_str(args, "speaker");
        assert(speaker);
        state->speaker = speaker;
    } else if (strcmp("draw_name", cmd) == 0) {
        char* name = get_arg_str(args, "name");
        assert(name);

        VisualLayer* layer = state->active_screen->active_layer;
        if (*name != '\0') {
            // scenario/macro.ks
            layer->pointer_pos.x = 75 + layer->margins.left;
            layer->pointer_pos.y = 1 + layer->margins.top;

            layer->font = Font_DroidSerif;
            layer->font.size = 16;

            size_t str_size = strlen(name) + 5;
            char* name_buf = malloc(str_size);
            snprintf(name_buf, str_size, "- %s -", name);

            create_text(state, name_buf);

            layer->font = state->active_screen->default_font;
        }

		layer->pointer_pos.x = 0 + layer->margins.left;
		layer->pointer_pos.y = 42 + layer->margins.top;

    } else if (strcmp("c", cmd) == 0) {
        char* text = get_arg_str(args, "text");

		RFont font = state->active_screen->active_layer->font;

		RVec2 size = r_measure_text(font, text);

		int pos = (state->canvas_size.x - size.x) / 2;
		state->active_screen->active_layer->pointer_pos.x = pos;

        assert(text);
        create_text(state, text);
    } else {
		for (int i=0; i<state->macros.length; i++) {
			Macro* macro = v_get(&state->macros, i);
			if (strcmp(macro->name, cmd) != 0) continue;

			printf("Need to execute macro '%s':\n", macro->name);

			for (int j=0; j<macro->children.length; j++) {
				printf(" - ");
				BaseNode* child = v_get(&macro->children, j);
				print_node(child, "exec-macro");
			}

			push_to_callstack(state);
			state->target_nodes = &macro->children;
			state->node_idx = 0;
			return true;
		}
	}

    return false;
}

void stop_transition(FataState* state, VisualScreen* screen) {
	unload_page_textures(&screen->fore);

	copy_page(&screen->fore, &screen->back);
	init_page(screen, &screen->back);
	
	screen->fore.name = "fore";
    screen->back.name = "back";

	state->transition_max_ms = 0.0f;
	state->transition_remaining_ms = 0.0f;
	state->wait_for_transition = false;
	state->can_skip_transition = false;
}

void frame_work(FataState* state, double delta_ms) {
	bool dont = false;
	for (int i=0; i<state->screens.length; i++) {
		VisualScreen* screen = v_get(&state->screens, i);

		if (screen->trans_remaining_ms <= 0.0f) return;

		screen->trans_remaining_ms -= delta_ms;

		if (screen->trans_remaining_ms <= 0.0f) {
			stop_transition(scree);
		} else if (state->wait_for_transition) {
			dont = true;
		}
	}

	if (dont) return;

    if (state->stopped) return;
    if (state->stopped_until_click) return;

    if (state->wait_timer_ms > 0.0f) {
        state->wait_timer_ms -= delta_ms;
        if (state->wait_timer_ms < 0.0f) {
            state->wait_timer_ms = 0.0f;
        } else {
            return;
        }
    }

    while (state->node_idx < state->target_nodes->length) {
        r_step(state);

        BaseNode* node = v_get(state->target_nodes, state->node_idx++);
		print_node(node, "exec");

		if (node->type == NODE_COMMAND) {
			bool showstopper = run_command((CommandNode*)node, state);
            if (showstopper) return;
		} else if (node->type == NODE_TEXT) {
            TextNode* text_node = (TextNode*)node;
			create_text(state, text_node->text);
        }
    }

	// We are out of nodes in the current execution context. See if we can pop.
	if (state->call_stack.length) {
		return_from_callstack(state);
		return;
	}

	// Okay now it's a problem
    printf("[exec] Done????\n");
	exit(0);
}

int main() {
	//printf("FataMoru!! ^-^\n");
    //

    FataState state = {0};

    state.audio.bgm.valid = false;
	state.script_name = NULL;
    state.wait_timer_ms = 0.0f;
    state.last_time_ms = r_time_ms();
    state.transition_remaining_ms = 0.0f;
    state.transition_max_ms = 0.0f;
	state.can_skip_transition = false;
	state.can_skip_wait = false;
	state.call_stack = v_new();
    state.macros = v_new();
	state.canvas_size = (RVec2) { 800, 600 };

    state.active_screen = &state.primary_screen_storage;
    r_init(&state);
    // Platform must assign screen with init_screen!
    assert(state.primary_screen_storage.valid);

    jump_to_point(&state, "bootstrap.ks", NULL);

	Font_DroidSerif = r_load_font(find_font("DroidSerif"));
	Font_LibreBaskerville = r_load_font(find_font("LibreBaskerville"));
	Font_LibreBaskerville.size = 18;
	Font_LibreBaskerville.spacing = 8;

	// See system/Config.tjs
	state.active_screen->default_font = Font_LibreBaskerville;
	// state.visual.default_font.shadow_color = GetColor(0x2b2b2b);
	// state.visual.default_font.shadow_enabled = true
	// TODO: Edge

	r_post_init(&state);

    while (r_main_loop(&state)) {
        double now = r_time_ms();
        double delta_ms = now - state.last_time_ms;

        // delta_ms *= 10.0;

        state.last_time_ms = now;

		if (r_get_click() || r_get_skip_held()) {
			state.stopped_until_click = false;

			if (
				state.can_skip_transition &&
				state.transition_remaining_ms > 0.0f
			) {
				stop_transition(&state);
			} else if (
				state.can_skip_wait &&
				state.wait_timer_ms > 0.0f
			) {
				state.wait_timer_ms = 0.0f;
				state.can_skip_wait = false;
			}
		}

        frame_work(&state, delta_ms);

		draw_everything(&state, &state.primary_screen_storage);
    }

    return 0;
}
