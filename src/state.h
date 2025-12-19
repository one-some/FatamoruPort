#pragma once
#include "mem.h"
#include "vector.h"
#include "visual.h"
#include "audio.h"

typedef struct FataState {
    MemArena static_arena;

    Vector nodes;
    Vector* target_nodes;

	char* script_name;
    double last_time_ms;
    float wait_timer_ms;
    int node_idx;

    AudioManager audio;
    VisualScreen primary_screen_storage;
    VisualScreen* active_screen;
	Vector screens;

	bool can_skip_wait;
	bool can_skip_transition;

    bool wait_for_transition;

    bool stopped;
	bool stopped_until_click;

	Vector call_stack;
    Vector macros;

    char* speaker;
	RVec2 canvas_size;
} FataState;

typedef struct {
	char* script_name;
	int node_idx;
    Vector* target_nodes;
} ScriptLocation;

typedef struct {
    char* name;
    Vector children;
} Macro;

void jump_to_point(FataState* state, char* path, char* label_target);
void push_to_callstack(FataState* state);
void return_from_callstack(FataState* state);
