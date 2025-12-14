#pragma once

#include "raylib.h"
#include "vector.h"
#include "audio.h"

typedef struct FataState FataState;

typedef struct {
	int x;
	int y;
} Vec2;

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} Margins;

typedef struct {
	Font resource;
	int size;
    int spacing;
	Color color;
} FontConfig;

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

void init_layer(FataState* state, VisualLayer* layer, char* name);
void init_page(FataState* state, VisualPage* page);

void copy_layer(VisualLayer* dest, VisualLayer* src);
void copy_page(VisualPage* dest, VisualPage* src);

void draw_layer(FataState* state, VisualLayer* layer, Vector2 mouse_pos);
void draw_page(FataState* state, VisualPage* page, Vector2 mouse_pos);

void unload_page_textures(VisualPage* page);
VisualLayer* get_layer(FataState* state, char* layer_name, char* page_name);
