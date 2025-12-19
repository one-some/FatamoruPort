#pragma once

#include "rosetta.h"
#include "vector.h"
#include "audio.h"

typedef struct FataState FataState;

typedef enum {
    DRAW_TEXTURES = 1,
    DRAW_CHILDREN = 2,
} DrawFlag;

#define DRAW_ALL (DRAW_TEXTURES | DRAW_CHILDREN)


typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} Margins;

typedef struct {
    RTexture texture;
	RVec2 texture_offset;

	char* name;
	Vector children;

	// Wasteful! Only used on message layers
	RFont font;

	Margins margins;
	RVec2 pointer_pos;
} VisualLayer;

typedef struct {
	char* name;
    RRenderTexture render_target;

    VisualLayer base_layer;
    VisualLayer layer_zero;
    VisualLayer layer_one;
    VisualLayer layer_two;
    VisualLayer message_layer_zero;
    VisualLayer message_layer_one;
} VisualPage;

typedef struct VisualScreen {
    char* name;

	VisualPage fore;
	VisualPage back;
	VisualLayer* active_layer;

	RFont default_font;
	RVec2 size;

    bool valid;
} VisualScreen;

void init_layer(VisualScreen* screen, VisualLayer* layer, char* name);
void init_page(VisualScreen* screen, VisualPage* page);
void init_screen(VisualScreen* screen, char* name, RVec2 size);

void copy_layer(VisualLayer* dest, VisualLayer* src);
void copy_page(VisualPage* dest, VisualPage* src);

void draw_layer(FataState* state, VisualLayer* layer, int flags);
void draw_page(FataState* state, VisualPage* page);
void draw_screen(FataState* state, VisualScreen* screen);
void draw_everything(FataState* state, VisualScreen* screen);

void unload_page_textures(VisualPage* page);
VisualLayer* get_layer(FataState* state, char* layer_name, char* page_name);
