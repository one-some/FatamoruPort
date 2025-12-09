// TODO: Arena allocater
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "raylib.h"
#include "vector.h"
#include "parse.h"

const int NO_ARG_INT = 1 << 31;

typedef struct {
    Texture2D texture;
    // Queen
    bool valid;
} VisualLayer;

typedef struct {
    VisualLayer base_layer;
} VisualScene;

typedef struct {
    Vector nodes;
    double last_time;
    float wait_timer;
    int node_idx;
    VisualScene visual;
} FataState;

typedef struct {
    char* key;
    char* value;
} CommandArg;

void load(FataState* state, char* path) {
    // TODO: Deep free
    state->nodes = v_new();
    state->node_idx = 0;

    // WARNING: When (if) we free later, the ptr will be gone! Save it!!
	char* src = load_src(path);

	while (*src) {
        BaseNode* node = parse_one(&src);
		if (!node) continue;

        v_append(&state->nodes, node);
	}
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

void strip_quotes(char* str) {
    // hehe not mine. I'm evil and lazy
    size_t len = strlen(str);

    if (len > 1 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
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

bool run_command(CommandNode* command, FataState* state) {
    // returns true if showstopper
	Vector parts = slice_command(command->cmd);

	char* cmd = v_pop(&parts, 0);
    Vector args = v_new();

    for (int i=0; i<parts.length; i++) {
        char* arg_str = v_get(&parts, i);

        CommandArg* arg = malloc(sizeof(CommandArg));
        arg->key = malloc(64);
        arg->value = malloc(64);

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
        v_append(&args, arg);
    }


	if (strcmp("wait", cmd) == 0) {
        int time_ms = get_arg_int(&args, "time");
        assert(time_ms != NO_ARG_INT);
        state->wait_timer = (float)time_ms;
        return true;
    } else if (strcmp("image", cmd) == 0) {
        // image storage="blacksozai" layer="base" page="fore"
        char* storage = get_arg_str(&args, "storage");
        assert(storage);

        char* path = find_image(storage);
        assert(path);

        VisualLayer* layer = NULL;
        char* layer_str = get_arg_str(&args, "layer");
        assert(layer_str);

        if (strcmp(layer_str, "base") == 0) {
            layer = &state->visual.base_layer;
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
        char* storage = get_arg_str(&args, "storage");
        assert(storage);

        char path_buffer[256];
        snprintf(path_buffer, sizeof(path_buffer), "cache/scenario/%s", storage);

        load(state, path_buffer);
        return true;
    }

    return false;
}

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

void frame_work(FataState* state) {
    if (state->wait_timer > 0.0f) return;

    while (state->node_idx < state->nodes.length) {
        BaseNode* node = v_get(&state->nodes, state->node_idx++);
		print_node(node);

		if (node->type == NODE_COMMAND) {
			bool showstopper = run_command((CommandNode*)node, state);
            if (showstopper) return;
		}
    }
}

int main() {
	printf("FataMoru!! ^-^\n");

    FataState state;

    state.visual.base_layer.valid = false;

    state.wait_timer = 0.0f;
    state.last_time = get_time_ms();

    load(&state, "cache/scenario/first.ks");

    // Raylib stuff...
    InitWindow(800, 600, "The House in Fata Morgana");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        double now = get_time_ms();
        double delta_ms = now - state.last_time;
        state.last_time = now;

        if (state.wait_timer > 0.0f) {
            state.wait_timer -= delta_ms;
            if (state.wait_timer < 0.0f) state.wait_timer = 0.0f;
            printf("Wait! %f\n", state.wait_timer);
        }

        frame_work(&state);

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawText("FatamoruPORT! By Claire :3", 0, 0, 20, BLACK);

        if (state.visual.base_layer.valid) {
            printf("Drawing..\n");
            DrawTexture(state.visual.base_layer.texture, 0, 0, WHITE);
        }

        EndDrawing();
    }
}
