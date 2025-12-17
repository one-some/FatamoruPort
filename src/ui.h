#pragma once

#include "rosetta.h"
#include "visual.h"

static RFont Font_DroidSerif;
static RFont Font_LibreBaskerville;

typedef struct FataState FataState;

typedef enum {
    VO_BUTTON,
    VO_TEXT,
} UIObjectType;

typedef struct {
    UIObjectType type;
} UIObject;

typedef struct {
    UIObject base;

	RVec2 position;
	char* target;
	RTexture texture;

    bool hovered;
    RSound enter_se;
} ButtonObject;

typedef struct {
    UIObject base;

	RVec2 position;
	RTextInstance text_instance;
} TextObject;

void create_text(FataState* state, char* text);
