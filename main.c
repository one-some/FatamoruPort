// TODO: Arena allocater
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "rlgl.h"
#include "raylib.h"
#include "vector.h"
#include "parse.h"

static Font Font_DroidSerif;
static Font Font_LibreBaskerville;

const char* useful_scripts[] = {
	"macro.ks",
	"macros_sprites.ks",
	NULL
};

typedef struct {
    bool valid;
    Sound resource;
} AudioTrack;

typedef struct {
    AudioTrack bgm;
} AudioManager;

typedef struct {
	int x;
	int y;
} Vec2;

typedef struct {
    Texture2D normal;
    Texture2D pressed;
    Texture2D hover;
} ButtonTextureCollection;

typedef enum {
    VO_BUTTON,
    VO_TEXT,
} VisualObjectType;

typedef struct {
    VisualObjectType type;
} VisualObject;

typedef struct {
	Font resource;
	int size;
    int spacing;
	Color color;
} FontConfig;

typedef struct {
    VisualObject base;

	Vec2 position;
	char* target;
	ButtonTextureCollection textures;

    AudioTrack enter_se;
} ButtonObject;

typedef struct {
    VisualObject base;

	Vec2 position;
    char* text;
	FontConfig font;
} TextObject;

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} Margins;

typedef struct {
    Texture2D texture;
	Vec2 texture_offset;
    bool texture_valid;

	char* name;
	Vector children;

	// Wasteful! Only used on message layers
	FontConfig font;
	Margins margins;
	Vec2 pointer_pos;
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

	FontConfig default_font;
} VisualManager;

typedef struct {
	char* script_path;
	int node_idx;
    Vector* target_nodes;
} ScriptLocation;

typedef struct {
    char* name;
    Vector children;
} Macro;

typedef struct {
    Vector nodes;
    Vector* target_nodes;

	char* script_path;
    double last_time_ms;
    float wait_timer_ms;
    int node_idx;

    AudioManager audio;
    VisualManager visual;

	bool can_skip_wait;
	bool can_skip_transition;

    bool wait_for_transition;
    float transition_max_ms;
    float transition_remaining_ms;

    bool stopped;
	bool stopped_until_click;

	Vector call_stack;
    Vector macros;

    char* speaker;
	Vec2 window_size;
} FataState;

void breakpoint(FataState* state) {
    state->stopped_until_click = true;
}

void copy_layer(VisualLayer* dest, VisualLayer* src) {
	// Blehhhh....... :/

	Margins margin_keep = dest->margins;

	*dest = *src;

	dest->margins = margin_keep;
}

void copy_page(VisualPage* dest, VisualPage* src) {
	copy_layer(&dest->base_layer, &src->base_layer);
    copy_layer(&dest->layer_zero, &src->layer_zero);
    copy_layer(&dest->layer_one, &src->layer_one);
    copy_layer(&dest->layer_two, &src->layer_two);
    copy_layer(&dest->message_layer_zero, &src->message_layer_zero);
    copy_layer(&dest->message_layer_one, &src->message_layer_one);
}

void load(FataState* state, char* path, char* label_target) {
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

    load(state, where->script_path, NULL);
    state->node_idx = where->node_idx;

    // This is gonna bite me in the butt later
    state->target_nodes = where->target_nodes;
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

char* find_bgm(char* storage) {
	const char* patterns[] = {
		"./cache/bgm/%s.ogg",
		NULL
	};
	return find_file(storage, patterns);
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

AudioTrack load_track(char* path) {
    return (AudioTrack) {
        .valid = true,
        .resource = LoadSound(path)
    };
}

void* play_one_off_se(char* path) {
    // GOD THIS IS BAD!
    PlaySound(load_track(path).resource);
}

VisualLayer* get_layer(FataState* state, char* layer_name, char* page_name) {
	assert(layer_name);

	// HACK: OKAY SO LETS JUST LIE THEN I GUESS...
	if (!page_name) page_name = "fore";
	assert(page_name);

	VisualPage* page = NULL;
	if (strcmp("fore", page_name) == 0) {
		page = &state->visual.fore;
	} else if (strcmp("back", page_name) == 0) {
		page = &state->visual.back;
	}
	assert(page);

    VisualLayer* layers[] = {
        &page->base_layer, &page->layer_zero, &page->layer_one, 
        &page->layer_two, &page->message_layer_zero, &page->message_layer_one, NULL
    };

	for (int i=0; layers[i]; i++) {
		if (strcmp(layers[i]->name, layer_name) != 0) continue;
		return layers[i];
	}

	assert(false);
}

void* create_text(FataState* state, char* text) {
	TextObject* text_object = malloc(sizeof(TextObject));
	text_object->base = (VisualObject) { .type = VO_TEXT };
	text_object->text = text;
	text_object->font = state->visual.active_layer->font;
	text_object->position = state->visual.active_layer->pointer_pos;

	Vector2 size = MeasureTextEx(
		text_object->font.resource,
		text,
		(float)text_object->font.size,
		(float)text_object->font.spacing
	);

	state->visual.active_layer->pointer_pos.x += size.x;

	printf("Making text on %s\n", state->visual.active_layer->name);

	v_append(&state->visual.active_layer->children, text_object);
}

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
			printf("[err] Weird layer %s\n", layer_str);
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
    } else if (strcmp("playbgm", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        assert(storage);

        char* path = find_bgm(storage);
        assert(path);

        if (state->audio.bgm.valid) {
            StopSound(state->audio.bgm.resource);
            UnloadSound(state->audio.bgm.resource);
        }

        state->audio.bgm.valid = true;
        state->audio.bgm.resource = LoadSound(path);
        PlaySound(state->audio.bgm.resource);
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
		VisualLayer* layer = state->visual.active_layer;

        int x = get_arg_int(args, "x");
        if (x != NO_ARG_INT) {
			layer->pointer_pos.x = x + layer->margins.left;
		}

        int y = get_arg_int(args, "y");
        if (y != NO_ARG_INT) {
			layer->pointer_pos.y = y + layer->margins.top;
		}
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

        char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

		ButtonObject* button = malloc(sizeof(ButtonObject));
        button->base = (VisualObject) { .type = VO_BUTTON };
		button->target = target;
		button->position = state->visual.active_layer->pointer_pos;

        // Blehhh!!
        button->enter_se.valid = false;

        char* enter_se_storage = get_arg_str(args, "enterse");
        if (enter_se_storage) {
			printf("TODO\n");
            // button->enter_se = load_track(find_bgm(enter_se_storage));
        }

        button->textures = read_button_textures(path);

		v_append(&state->visual.active_layer->children, button);
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

		char* path = find_script(storage);
		assert(path);

		char* target = get_arg_str(args, "target");
        if (target) {
            assert(target[0] == '*');
            target++;
        }

        push_to_callstack(state);

        load(state, path, target);
		return true;
    } else if (strcmp("return", cmd) == 0) {
        return_from_callstack(state);
		return true;
    } else if (strcmp("cm", cmd) == 0) {
		VisualLayer* layers[] = {
			&state->visual.fore.message_layer_zero,
			&state->visual.fore.message_layer_one,
			&state->visual.back.message_layer_zero,
			&state->visual.back.message_layer_one,
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
		FontConfig* font = &state->visual.active_layer->font;

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
		copy_page(&state->visual.back, &state->visual.fore);
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

			Image img = LoadImage(path);
			layer->texture = LoadTextureFromImage(img);
			layer->texture_valid = true;
			UnloadImage(img);
		}
    } else if (strcmp("title", cmd) == 0) {
        char* name = get_arg_str(args, "name");
		assert(name);
		SetWindowTitle(name);
    } else if (strcmp("r", cmd) == 0) {
		VisualLayer* layer = state->visual.active_layer;
		layer->pointer_pos.x = layer->margins.left;

		// TODO: Spacing...
		layer->pointer_pos.y += layer->font.size;
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

		// scenario/macro.ks
		VisualLayer* layer = state->visual.active_layer;
		layer->pointer_pos.x = 75 + layer->margins.left;
		layer->pointer_pos.y = 1 + layer->margins.top;

		layer->font.resource = Font_DroidSerif;
		layer->font.size = 16;

        size_t str_size = strlen(name) + 5;
        char* name_buf = malloc(str_size);
        snprintf(name_buf, str_size, "- %s -", name);

		create_text(state, name_buf);

		layer->font = state->visual.default_font;

		layer->pointer_pos.x = 0 + layer->margins.left;
		layer->pointer_pos.y = 42 + layer->margins.top;

    } else if (strcmp("c", cmd) == 0) {
        char* text = get_arg_str(args, "text");

		FontConfig* font = &state->visual.active_layer->font;

		Vector2 size = MeasureTextEx(
			font->resource,
			text,
			(float)font->size,
            0.0f
		);

		int pos = (state->window_size.x - size.x) / 2;
		state->visual.active_layer->pointer_pos.x = pos;

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

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

void unload_page_textures(VisualPage* page) {
	// TODO: FIXME!!~!
	return;

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

void init_layer(FataState* state, VisualLayer* layer, char* name) {
	layer->texture_valid = false;
	layer->texture_offset = (Vec2) { 0, 0 };
	layer->name = name;
	layer->children = v_new();
	layer->pointer_pos = (Vec2) { 0, 0 };

	layer->font = state->visual.default_font;
	layer->margins = (Margins) {
		.left = 70,
		.top = 0,
		.right = 70,
		.bottom = 20
	};
}

void init_page(FataState* state, VisualPage* page) {
	init_layer(state, &page->base_layer, "base");
	init_layer(state, &page->layer_zero, "0");
	init_layer(state, &page->layer_one, "1");
	init_layer(state, &page->layer_two, "2");
	init_layer(state, &page->message_layer_zero, "message0");
	init_layer(state, &page->message_layer_one, "message1");
}

void stop_transition(FataState* state) {
	unload_page_textures(&state->visual.fore);

	copy_page(&state->visual.fore, &state->visual.back);
	init_page(state, &state->visual.back);
	
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

    while (state->node_idx < state->target_nodes->length) {
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
        VisualObject* obj = v_get(&layer->children, i);
        assert(obj);

        if (obj->type == VO_BUTTON) {
            ButtonObject* button = (ButtonObject*)obj;

            int pos_x = button->position.x;
            int pos_y = button->position.y;

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

            // DrawRectangleLinesEx(rect, 1.0f, RED);

            if (mouse_inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                load(state, NULL, button->target);
            }
        } else if (obj->type == VO_TEXT) {
            TextObject* text_obj = (TextObject*)obj;
			FontConfig* font = &state->visual.active_layer->font;

			DrawTextEx(
				text_obj->font.resource,
				text_obj->text,
				(Vector2) { text_obj->position.x, text_obj->position.y },
				(float)font->size,
                0.0f,
				font->color
			);
        } else {
            assert(false);
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

	state.visual.fore.name = "fore";
	state.visual.back.name = "back";
	init_page(&state, &state.visual.fore);
	init_page(&state, &state.visual.back);

    state.audio.bgm.valid = false;
	state.script_path = NULL;
    state.wait_timer_ms = 0.0f;
    state.last_time_ms = get_time_ms();
    state.transition_remaining_ms = 0.0f;
    state.transition_max_ms = 0.0f;
	state.can_skip_transition = false;
	state.can_skip_wait = false;
	state.call_stack = v_new();
    state.macros = v_new();
	state.window_size = (Vec2) { 800, 600 };

    load(&state, "./static/bootstrap.ks", NULL);

    SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(
		state.window_size.x,
		state.window_size.y,
		"The House in Fata Morgana"
	);
    SetTargetFPS(60);
    
    InitAudioDevice();

	Font_DroidSerif = LoadFont("./static/DroidSerif.ttf");
    SetTextureFilter(Font_DroidSerif.texture, TEXTURE_FILTER_BILINEAR);
	Font_LibreBaskerville = LoadFont("./static/LibreBaskerville.ttf");
    SetTextureFilter(Font_LibreBaskerville.texture, TEXTURE_FILTER_BILINEAR);

	// See system/Config.tjs
	state.visual.default_font.resource = Font_LibreBaskerville;
	state.visual.default_font.size = 18;
	state.visual.default_font.spacing = 8;
	// state.visual.default_font.shadow_color = GetColor(0x2b2b2b);
	// state.visual.default_font.shadow_enabled = true
	// TODO: Edge
	state.visual.default_font.color = WHITE;


    RenderTexture2D fore_target = LoadRenderTexture(800, 600);
    RenderTexture2D back_target = LoadRenderTexture(800, 600);

    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE, RL_FUNC_ADD, RL_MAX);
    while (!WindowShouldClose()) {
        double now = get_time_ms();
        double delta_ms = now - state.last_time_ms;

        // delta_ms *= 10.0;

        state.last_time_ms = now;

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_LEFT_CONTROL)) {
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
            BeginBlendMode(BLEND_CUSTOM_SEPARATE);

			ClearBackground(BLANK);
            draw_page(&state, &state.visual.fore, mouse_pos);

            EndBlendMode();
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
                BeginBlendMode(BLEND_CUSTOM_SEPARATE);

				ClearBackground(BLANK);
                draw_page(&state, &state.visual.back, mouse_pos);

                EndBlendMode();
            EndTextureMode();
        }

        BeginDrawing();

            ClearBackground(GREEN);
            DrawText("FatamoruPORT! By Claire :3\nIf u can see this something is not right", 0, 0, 20, BLACK);

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
