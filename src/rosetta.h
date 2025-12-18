#pragma once

extern int __real_printf(const char* format, ...);

#include <stdio.h>

#ifndef PLATFORM_3DS
    #include <assert.h>
#else
    #include <3ds.h>
    
    #undef assert 

    #define assert(_Expression) (void)( (!!(_Expression)) || (_3ds_assert(#_Expression, __FILE__, __LINE__), 0) )

    __attribute__((unused)) static void _3ds_assert(const char* expr, const char* file, int line) {

        printf("\x1b[31m\n");
		printf("\n\n");
		// There is some weirdness going on with svcOutputDebugString here... not everything is printed..
		printf("! ASSERTION FAILED !\n");
        printf("[assert] File: %s\n", file);
        printf("[assert] Line: %d\n", line);
        printf("[assert] Expr: %s", expr);
		printf("\x1b[0m");
        
        while (true) {
            gspWaitForVBlank(); 
        }
    }
#endif


#include <stddef.h>
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
	RVec2 size;
} RRenderTexture;

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

typedef struct {
	void* resource;
	RFont font;
} RTextInstance;

void r_init(FataState* state);
void r_post_init(FataState* state);
void r_shutdown();
void r_dbgout(char* string, size_t length);
void r_step(FataState* state);

char* r_jump_hook(FataState* state, char* storage);

bool r_main_loop(FataState* state);

void r_begin_frame(FataState* state);
void r_end_frame();

void r_clear_frame(RColor color);

double r_time_ms();

RTexture r_load_texture(char* path);
void r_unload_texture(RTexture texture);

// SetTextureFilter(fore_target.texture, TEXTURE_FILTER_BILINEAR);
//SetTextureFilter(back_target.texture, TEXTURE_FILTER_BILINEAR);
RRenderTexture r_create_render_texture(RVec2 size);

void r_begin_render_texture_draw(RRenderTexture texture);
void r_end_render_texture_draw(RRenderTexture texture);


// Stupidly narrow because I remembered the project has a scope
void r_draw_render_texture(RRenderTexture texture, float alpha);

// Don't implement these shortcuts
void r_draw_texture(RTexture texture, RVec2 position);
void r_draw_texture_tint(RTexture texture, RVec2 position, RColor tint);

void r_draw_texture_tint_sample(RTexture texture, RVec2 position, RColor tint, RRect sample_rect);

// SetTextureFilter(Font_DroidSerif.texture, TEXTURE_FILTER_BILINEAR);
// SetTextureFilter(Font_LibreBaskerville.texture, TEXTURE_FILTER_BILINEAR);
RFont r_load_font(char* path);
void r_unload_font(RFont font);

RTextInstance r_create_text(char* string, RFont font);
void r_draw_text(RTextInstance text_instance, RVec2 position);

RVec2 r_measure_text(RFont font, char* text);

RSound r_load_sound(char* path);
void r_play_sound(RSound track);
void r_stop_sound(RSound track);
void r_unload_sound(RSound track);

bool r_get_click();
bool r_get_click_down();

bool r_get_skip_held();
RVec2 r_get_cursor_pos();

void r_set_window_title(char* title);
