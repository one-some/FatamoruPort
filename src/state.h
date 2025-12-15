#pragma once
#include "vector.h"
#include "visual.h"
#include "audio.h"

typedef struct FataState {
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
	RVec2 window_size;
	RVec2 canvas_size;
} FataState;

void load(FataState* state, char* path, char* label_target);
