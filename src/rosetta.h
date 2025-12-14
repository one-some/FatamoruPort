#pragma once

typedef struct FataState FataState;

typedef struct {
    void* resource;
} RTexture;

typedef struct {
    void* resource;
} RFont;

void r_init(FataState* state);
void r_shutdown();

bool r_main_loop(FataState* state);

void r_begin_frame();
void r_end_frame();

double r_time_ms();

RTexture r_load_texture(char* path);
void r_unload_texture(RTexture texture);

RFont r_load_font(char* path);
void r_unload_font(RFont font);
