#pragma once
#include "types.h"

typedef struct FataState FataState;

typedef struct {
    void* resource;
	bool valid;
	RVec2 size;
} RTexture;

typedef struct {
    void* resource;
	bool valid;
} RSound;

typedef struct {
    void* resource;
	int size;
	int spacing;
	RColor color;
} RFont;

void r_init(FataState* state);
void r_shutdown();

bool r_main_loop(FataState* state);

void r_begin_frame();
void r_end_frame();

double r_time_ms();

RTexture r_load_texture(char* path);
void r_unload_texture(RTexture texture);


// Don't implement these shortcuts
void r_draw_texture(RTexture texture, RVec2 position);
void r_draw_texture_tint(RTexture texture, RVec2 position, RColor tint);

void r_draw_texture_tint_sample(RTexture texture, RVec2 position, RColor tint, RRect sample_rect);

RFont r_load_font(char* path);
void r_unload_font(RFont font);
void r_draw_text(RFont font, char* text, RVec2 position);

RSound r_load_sound(char* path);
void r_play_sound(RSound track);

bool r_get_click();
RVec2 r_get_cursor_pos();
