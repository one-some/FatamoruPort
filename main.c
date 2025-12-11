// TODO: Arena allocater
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "raylib.h"
#include "vector.h"
#include "parse.h"

const char* useful_scripts[] = {
	"macro.ks",
	"macros_sprites.ks",
	NULL
};

typedef struct {
	int x;
	int y;
} Vec2;

typedef struct {
    Texture2D normal;
    Texture2D pressed;
    Texture2D hover;
} ButtonTextureCollection;

typedef struct {
	Vec2 position;
	char* target;
	ButtonTextureCollection textures;
} Button;

typedef struct {
    Texture2D texture;
	Vec2 texture_offset;
    bool texture_valid;

	char* name;
	Vector children;
} VisualLayer;

typedef struct {
	char* name;

    VisualLayer base_layer;
    VisualLayer layer_zero;
    VisualLayer layer_one;
    VisualLayer layer_two;
    VisualLayer message_layer_zero;
    VisualLayer message_layer_one;
} VisualPage;

typedef struct {
	VisualPage fore;
	VisualPage back;

	VisualLayer* active_layer;
	Vec2 pointer_pos;
} VisualScene;

typedef struct {
	char* script_path;
	int node_idx;
} CallstackEntry;

typedef struct {
	char* script_path;
    Vector nodes;
    double last_time_ms;
    float wait_timer_ms;
    int node_idx;
    VisualScene visual;

	bool can_skip_wait;
	bool can_skip_transition;

    bool wait_for_transition;
    float transition_max_ms;
    float transition_remaining_ms;

    bool stopped;
	bool stopped_until_click;

	Vector call_stack;
} FataState;

void load(FataState* state, char* path, char* label_target) {
	state->stopped = false;
	state->stopped_until_click = false;
    state->node_idx = 0;

    if (path) {
		state->script_path = path;

		printf("\n\nLOADINF FROM %s\n\n\n", path);
        state->nodes = v_new();

        // WARNING: When (if) we free later, the ptr will be gone! Save it!!
        char* src = load_src(path);

        while (*src) {
            BaseNode* node = parse_one(&src);
            if (!node) continue;

            v_append(&state->nodes, node);
        }
	}

	for (int i=0; i<state->nodes.length;i++) {
		BaseNode* node = v_get(&state->nodes, i);

		if (node->type == NODE_COMMAND) {
			CommandNode* cmd = (CommandNode*)node;

			if (cmd->data_type == CMD_DATA_MACRO) {
				printf("Macro:\n");
				Vector* macro_children = cmd->data;
				assert(macro_children);

				for (int i=0; i<macro_children->length; i++) {
					BaseNode* node = v_get(macro_children, i);
					print_node(node);
				}
			}
		}

		if (label_target && node->type == NODE_LABEL) {

			// Set node_idx to this label right off the bat
			LabelNode* label = (LabelNode*)node;
			printf("Heyyy maybe %s\n", label->label_id);

			if (strcmp(label->label_id, label_target) != 0) continue;

			printf("YESS!!\n");
			state->node_idx = i;

			label_target = NULL;
		}
	}

	if (label_target) {
		printf("[err] Couldn't find label %s\n", label_target);
		assert(!label_target);
	}

    printf("\n\nEND LOAD\n\n\n");

}

ButtonTextureCollection read_button_textures(char* path) {
	ButtonTextureCollection textures;

	Image img_base = LoadImage(path);

	int width = img_base.width / 3;

	Image working_img = ImageFromImage(
		img_base,
		(Rectangle) { 0, 0, width, img_base.height }
	);
	textures.normal = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	working_img = ImageFromImage(
		img_base,
		(Rectangle) { width, 0, width, img_base.height }
	);
	textures.pressed = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	working_img = ImageFromImage(
		img_base,
		(Rectangle) { width * 2, 0, width, img_base.height }
	);
	textures.hover = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	return textures;
}

char* find_file(char* storage, const char* patterns[]) {
    for (int i=0; ; i++) {
        if (!patterns[i]) break;

        char path_buffer[256];
        snprintf(path_buffer, sizeof(path_buffer), patterns[i], storage);

        FILE *fp = fopen(path_buffer, "r");
        if (fp) {
            fclose(fp);
            return strdup(path_buffer);
        }
    }

    assert(false);
}

char* find_image(char* storage) {
	const char* patterns[] = {
		"./cache/bgimage/%s.png",
		"./cache/bgimage/%s.jpg",
		"./cache/image/%s.png",
		"./cache/image/%s.jpg",
		"./cache/fgimage/%s.png",
		"./cache/fgimage/%s.jpg",
		NULL
	};
	return find_file(storage, patterns);
}

char* find_script(char* storage) {
	const char* patterns[] = {
		"./cache/scenario/%s",
		"./cache/others/%s",
		"./cache/system/%s",
		NULL
	};
	return find_file(storage, patterns);
}

bool run_command(CommandNode* command, FataState* state) {
    // returns true if showstopper
    Vector* args = &command->args;
	char* cmd = ((CommandArg*)v_get(args, 0))->key;

	if (strcmp("wait", cmd) == 0) {
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
        // image storage="blacksozai" layer="base" page="fore"
        char* storage = get_arg_str(args, "storage");
        assert(storage);

        char* path = find_image(storage);
        assert(path);

        VisualPage* page = NULL;
        char* page_str = get_arg_str(args, "page");
        assert(page_str);

        if (strcmp(page_str, "fore") == 0) {
            page = &state->visual.fore;
        } else if (strcmp(page_str, "back") == 0) {
            page = &state->visual.back;
        } else {
            assert(false);
        }

        VisualLayer* layer = NULL;
        char* layer_str = get_arg_str(args, "layer");
        assert(layer_str);

        if (strcmp(layer_str, "base") == 0) {
            layer = &page->base_layer;
        } else if (strcmp(layer_str, "0") == 0) {
            layer = &page->layer_zero;
        } else if (strcmp(layer_str, "1") == 0) {
            layer = &page->layer_one;
        } else if (strcmp(layer_str, "2") == 0) {
            layer = &page->layer_two;
        } else if (strcmp(layer_str, "message0") == 0) {
            layer = &page->message_layer_zero;
        } else if (strcmp(layer_str, "message1") == 0) {
            layer = &page->message_layer_one;
        } else {
			printf("Weird layer %s\n", layer_str);
            assert(false);
        }

        if (layer->texture_valid) {
            UnloadTexture(layer->texture);
        }

        Image img = LoadImage(path);
        layer->texture = LoadTextureFromImage(img);
        layer->texture_valid = true;
        UnloadImage(img);

        int top_px = get_arg_int(args, "top");
		if (top_px != NO_ARG_INT) {
			layer->texture_offset.y = top_px;
		}

        int left_px = get_arg_int(args, "left");
		if (left_px != NO_ARG_INT) {
			layer->texture_offset.x = left_px;
		}

        printf("Load from: %s\n", path);
    } else if (strcmp("jump", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        char* path = storage ? find_script(storage) : NULL;

        char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

        load(state, path, target);
        return true;
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
        int x = get_arg_int(args, "x");
        assert(x != NO_ARG_INT);
		state->visual.pointer_pos.x = x;

        int y = get_arg_int(args, "y");
        assert(y != NO_ARG_INT);
		state->visual.pointer_pos.y = y;
    } else if (strcmp("current", cmd) == 0) {
		char* layer = get_arg_str(args, "layer");
		assert(layer);

		if (strcmp("message0", layer) == 0) {
			state->visual.active_layer = &state->visual.fore.message_layer_zero;
		} else if (strcmp("message1", layer) == 0) {
			state->visual.active_layer = &state->visual.fore.message_layer_one;
		} else {
			assert(false);
		}
    } else if (strcmp("button", cmd) == 0) {
		// button graphic:選択ライン hint:"Exit the game." target:*end enterse:button
		char* storage = get_arg_str(args, "graphic");
		assert(storage);
        char* path = find_image(storage);
        assert(path);

		printf(
			"Creating button from path: '%s' at (%d, %d)\n",
			path,
			state->visual.pointer_pos.x,
			state->visual.pointer_pos.y
		);

        char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

		Button* button = malloc(sizeof(Button));
		button->target = target;
		button->position = state->visual.pointer_pos;

        button->textures = read_button_textures(path);

		v_append(&state->visual.active_layer->children, button);
    } else if (strcmp("close", cmd) == 0) {
		printf("Goodbye from The House in Fata Morgana...\n");
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
			printf("Ignoring call to '%s'--not cool.\n", storage);
			return false;
		}

		char* path = find_script(storage);
		assert(path);

		char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

		CallstackEntry* where = malloc(sizeof(CallstackEntry));
		where->script_path = state->script_path;
		where->node_idx = state->node_idx;
		v_append(&state->call_stack, where);

        load(state, path, target);
		return true;
    } else if (strcmp("return", cmd) == 0) {
		assert(state->call_stack.length);
		CallstackEntry* where = v_get(&state->call_stack, state->call_stack.length - 1);
		assert(where);

        load(state, where->script_path, NULL);
		state->node_idx = where->node_idx;
		return true;
	}

    return false;
}

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

void unload_page_textures(VisualPage* page) {
    VisualLayer* layers[] = {
        &page->base_layer, &page->layer_zero, &page->layer_one, 
        &page->layer_two, &page->message_layer_zero, &page->message_layer_one
    };

    for (int i = 0; i < 6; i++) {
        if (layers[i]->texture_valid) {
            UnloadTexture(layers[i]->texture);
            layers[i]->texture_valid = false;
        }
    }
}

void init_layer(VisualLayer* layer, char* name) {
	layer->texture_valid = false;
	layer->texture_offset = (Vec2) { 0, 0 };
	layer->name = name;
	layer->children = v_new();
}

void init_page(VisualPage* page) {
	init_layer(&page->base_layer, "base");
	init_layer(&page->layer_zero, "0");
	init_layer(&page->layer_one, "1");
	init_layer(&page->layer_two, "2");
	init_layer(&page->message_layer_zero, "message0");
	init_layer(&page->message_layer_one, "message1");
}

void stop_transition(FataState* state) {
	unload_page_textures(&state->visual.fore);
	state->visual.fore = state->visual.back;
	init_page(&state->visual.back);
	
	state->visual.fore.name = "fore";
    state->visual.back.name = "back";

	state->transition_max_ms = 0.0f;
	state->transition_remaining_ms = 0.0f;
	state->wait_for_transition = false;
	state->can_skip_transition = false;
}

void frame_work(FataState* state, double delta_ms) {
    if (state->transition_remaining_ms > 0.0f) {
        state->transition_remaining_ms -= delta_ms;
        if (state->transition_remaining_ms <= 0.0f) {
			stop_transition(state);
        } else if (state->wait_for_transition) {
            return;
        }
    }

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

    while (state->node_idx < state->nodes.length) {
        BaseNode* node = v_get(&state->nodes, state->node_idx++);
		print_node(node);

		if (node->type == NODE_COMMAND) {
			bool showstopper = run_command((CommandNode*)node, state);
            if (showstopper) return;
		}
    }
}

void draw_layer(FataState* state, VisualLayer* layer, Vector2 mouse_pos) {
    if (layer->texture_valid) {
		//printf("Drawing a texture on '%s'\n", layer->name);
		DrawTexture(
			layer->texture,
			layer->texture_offset.x,
			layer->texture_offset.y,
			WHITE
		);
	}

	for (int i=0; i<layer->children.length;i++) {
		Button* button = v_get(&layer->children, i);

		int pos_x = button->position.x;
		int pos_y = button->position.y;

		//printf(
		//	"Drawing button %d on '%s' at (%d, %d)\n",
		//	i,
		//	layer->name,
		//	pos_x,
		//	pos_y
		//);

		Rectangle rect = (Rectangle) {
			pos_x,
			pos_y,
			button->textures.normal.width,
			button->textures.normal.height
		};



		bool mouse_inside = CheckCollisionPointRec(mouse_pos, rect);

		DrawTexture(
			mouse_inside ? button->textures.hover : button->textures.normal,
			pos_x,
			pos_y,
			WHITE
		);

		DrawRectangleLinesEx(rect, 1.0f, RED);

		if (mouse_inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			load(state, NULL, button->target);
		}
	}
}

void draw_page(FataState* state, VisualPage* page, Vector2 mouse_pos) {
	//printf("Drawing Page: '%s'\n", page->name);
    draw_layer(state, &page->base_layer, mouse_pos);
    draw_layer(state, &page->layer_zero, mouse_pos);
    draw_layer(state, &page->layer_one, mouse_pos);
    draw_layer(state, &page->layer_two, mouse_pos);
    draw_layer(state, &page->message_layer_zero, mouse_pos);
    draw_layer(state, &page->message_layer_one, mouse_pos);
}

int main() {
	//printf("FataMoru!! ^-^\n");

    FataState state;

	state.visual.active_layer = &state.visual.fore.message_layer_zero;
	state.visual.pointer_pos = (Vec2) { 0, 0 };

	state.visual.fore.name = "fore";
	state.visual.back.name = "back";
	init_page(&state.visual.fore);
	init_page(&state.visual.back);

	state.script_path = NULL;
    state.wait_timer_ms = 0.0f;
    state.last_time_ms = get_time_ms();
    state.transition_remaining_ms = 0.0f;
    state.transition_max_ms = 0.0f;
	state.can_skip_transition = false;
	state.can_skip_wait = false;
	state.call_stack = v_new();

    load(&state, "cache/scenario/first.ks", NULL);

    // Raylib stuff...
    InitWindow(800, 600, "The House in Fata Morgana");
    SetTargetFPS(60);

    RenderTexture2D fore_target = LoadRenderTexture(800, 600);
    RenderTexture2D back_target = LoadRenderTexture(800, 600);

    while (!WindowShouldClose()) {
        double now = get_time_ms();
        double delta_ms = now - state.last_time_ms;

        // delta_ms *= 10.0;

        state.last_time_ms = now;

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

		Vector2 mouse_pos = GetMousePosition();

        frame_work(&state, delta_ms);

        BeginTextureMode(fore_target);
            draw_page(&state, &state.visual.fore, mouse_pos);
        EndTextureMode();

        // Transition if needed
        float fore_to_back_fade = 0.0;
        if (state.transition_max_ms > 0.0f) {
			//printf("TMax: %f ... TRem: %f\n", state.transition_max_ms, state.transition_remaining_ms);
            float trans_progress_ms = state.transition_max_ms - state.transition_remaining_ms;
            fore_to_back_fade = trans_progress_ms / state.transition_max_ms;
        }

        if (fore_to_back_fade > 0.0f) {
            BeginTextureMode(back_target);
                draw_page(&state, &state.visual.back, mouse_pos);
            EndTextureMode();
        }

        BeginDrawing();

            ClearBackground(MAGENTA);
            DrawText("FatamoruPORT! By Claire :3", 0, 0, 20, BLACK);

            if (fore_to_back_fade > 0.0f) {
                DrawTextureRec(
                    back_target.texture,
                    (Rectangle) { 0.0f, 0.0f, (float)back_target.texture.width, -(float)back_target.texture.height },
                    (Vector2) { 0, 0 },
                    WHITE
                );
            }

            // We gotta do this whole song and dance because it's flipped
            DrawTextureRec(
                fore_target.texture,
                (Rectangle) { 0.0f, 0.0f, (float)fore_target.texture.width, -(float)fore_target.texture.height },
                (Vector2) { 0, 0 },
                Fade(WHITE, 1.0f - fore_to_back_fade)
            );

        EndDrawing();
    }
}
