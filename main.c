// TODO: Arena allocater
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "raylib.h"
#include "vector.h"
#include "parse.h"

typedef struct {
    Texture2D texture;
    // Queen
    bool valid;
} VisualLayer;

typedef struct {
    VisualLayer base_layer;
} VisualPage;

typedef struct {
	VisualPage fore;
	VisualPage back;
} VisualScene;

typedef struct {
    Vector nodes;
    double last_time_ms;
    float wait_timer_ms;
    int node_idx;
    VisualScene visual;

    bool wait_for_transition;
    float transition_max_ms;
    float transition_remaining_ms;

    bool stopped;
} FataState;

void load(FataState* state, char* path, char* label_target) {
    printf("\n\nLOADINF FROM %s\n\n\n", path);

    state->node_idx = 0;

    if (path) {
        state->nodes = v_new();

        // WARNING: When (if) we free later, the ptr will be gone! Save it!!
        char* src = load_src(path);

        while (*src) {
            BaseNode* node = parse_one(&src);
            if (!node) continue;

            v_append(&state->nodes, node);
        }
    }

    if (label_target) {
        for (int i=0; i<state->nodes.length;i++) {
            BaseNode* node = v_get(&state->nodes, i);

            if (node->type != NODE_LABEL) continue;

            // Set node_idx to this label right off the bat
            LabelNode* label = (LabelNode*)node;
            printf("Heyyy maybe %s\n", label->label_id);

            if (strcmp(label->label_id, label_target) != 0) continue;

            printf("YESS!!\n");
            state->node_idx = i;

            label_target = NULL;
            break;
        }

        if (label_target) {
            printf("[err] Couldn't find label %s\n", label_target);
            assert(!label_target);
        }
    }
    printf("\n\nEND LOAD\n\n\n");

}

char* find_image(char* storage) {
    const char *patterns[] = {
        "./cache/bgimage/%s.png",
        "./cache/bgimage/%s.jpg",
        NULL
    };

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

bool run_command(CommandNode* command, FataState* state) {
    // returns true if showstopper
    Vector* args = &command->args;
	char* cmd = ((CommandArg*)v_get(args, 0))->key;

	if (strcmp("wait", cmd) == 0) {
        int time_ms = get_arg_int(args, "time");
        assert(time_ms != NO_ARG_INT);
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
        } else {
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            printf("TODO\n");
            // assert(false);
            layer = &page->base_layer;
        }

        if (layer->valid) {
            UnloadTexture(layer->texture);
        }

        Image img = LoadImage(path);
        layer->texture = LoadTextureFromImage(img);
        layer->valid = true;
        UnloadImage(img);

        printf("Load from: %s\n", path);
    } else if (strcmp("jump", cmd) == 0) {
        char* storage = get_arg_str(args, "storage");
        char* path = NULL;

        if (storage) {
            char path_buffer[256];
            snprintf(path_buffer, sizeof(path_buffer), "cache/scenario/%s", storage);
            path = path_buffer;
        }

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
        // TODO: canskip
        state->wait_for_transition = true;
        return true;
    } else if (strcmp("s", cmd) == 0) {
        // STOP!
        state->stopped = true;
        return true;
    }

    return false;
}

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

void frame_work(FataState* state, double delta_ms) {
    if (state->stopped) return;

    if (state->wait_timer_ms > 0.0f) {
        state->wait_timer_ms -= delta_ms;
        if (state->wait_timer_ms < 0.0f) {
            state->wait_timer_ms = 0.0f;
        } else {
            return;
        }
    }

    if (state->transition_remaining_ms > 0.0f) {
        state->transition_remaining_ms -= delta_ms;
        if (state->transition_remaining_ms < 0.0f) {
            state->transition_remaining_ms = 0.0f;
            state->wait_for_transition = false;
        } else if (state->wait_for_transition) {
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

void initalize_visual(FataState* state) {
	state->visual.fore.base_layer.valid = false;
	state->visual.back.base_layer.valid = false;
}

void draw_layer(VisualLayer* layer) {
    if (!layer->valid) return;

    DrawTexture(layer->texture, 0, 0, WHITE);
}

void draw_page(VisualPage* page) {
    draw_layer(&page->base_layer);
}

int main() {
	printf("FataMoru!! ^-^\n");

    FataState state;
	initalize_visual(&state);
    state.wait_timer_ms = 0.0f;
    state.last_time_ms = get_time_ms();
    state.transition_remaining_ms = 0.0f;
    state.transition_max_ms = 0.0f;
    state.stopped = false;
    load(&state, "cache/scenario/first.ks", NULL);

    // Raylib stuff...
    InitWindow(800, 600, "The House in Fata Morgana");
    SetTargetFPS(60);

    RenderTexture2D fore_target = LoadRenderTexture(800, 600);
    RenderTexture2D back_target = LoadRenderTexture(800, 600);

    while (!WindowShouldClose()) {
        double now = get_time_ms();
        double delta_ms = now - state.last_time_ms;

        delta_ms *= 10.0;

        state.last_time_ms = now;

        frame_work(&state, delta_ms);

        BeginTextureMode(fore_target);
            draw_page(&state.visual.fore);
        EndTextureMode();

        // Transition if needed
        float fore_to_back_fade = 0.0;
        if (state.transition_max_ms > 0.0f) {
            float trans_progress_ms = state.transition_max_ms - state.transition_remaining_ms;
            fore_to_back_fade = trans_progress_ms / state.transition_max_ms;
        }

        if (fore_to_back_fade > 0.0f) {
            BeginTextureMode(back_target);
                draw_page(&state.visual.back);
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
