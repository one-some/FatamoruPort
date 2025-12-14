#include "rosetta.h"

#include "raylib.h"
#include "rlgl.h"
#include "state.h"
#include <sys/time.h>

void r_init(FataState* state) {
    SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);

    InitWindow(
		state->window_size.x,
		state->window_size.y,
		"The House in Fata Morgana"
	);
    SetTargetFPS(60);
    
    InitAudioDevice();

    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE, RL_FUNC_ADD, RL_MAX);
}

bool r_main_loop(FataState* state) {
    bool keep_going = !WindowShouldClose();

    state->window_size.x = GetRenderWidth();
    state->window_size.y = GetRenderHeight();

    return keep_going;
}

void r_begin_frame() {
    BeginDrawing();
    ClearBackground(BLACK);
}

void r_end_frame() {
    EndDrawing();
}

double r_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

