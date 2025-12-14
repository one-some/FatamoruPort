#pragma once

#include "visual.h"
#include "raylib.h"

static Font Font_DroidSerif;
static Font Font_LibreBaskerville;

typedef struct FataState FataState;

// We need to get rid of this, these should be texture offsets from the atlas
typedef struct {
    Texture2D normal;
    Texture2D pressed;
    Texture2D hover;
} ButtonTextureCollection;

typedef enum {
    VO_BUTTON,
    VO_TEXT,
} UIObjectType;

typedef struct {
    UIObjectType type;
} UIObject;

typedef struct {
    UIObject base;

	Vec2 position;
	char* target;
	ButtonTextureCollection textures;

    bool hovered;
    AudioTrack enter_se;
} ButtonObject;

typedef struct {
    UIObject base;

	Vec2 position;
    char* text;
	FontConfig font;
} TextObject;

ButtonTextureCollection read_button_textures(char* path);
void* create_text(FataState* state, char* text);
