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

typedef enum {
	BUTTON_MOUSE_NONE,
	BUTTON_MOUSE_HOVER,
	BUTTON_MOUSE_DEPRESSED
} ButtonMouseState;

typedef enum {
	BUTTON_NATIVE = 1,
	BUTTON_DISABLED = 2,
} ButtonFlag;

typedef struct {
    UIObject base;

	RVec2 position;
	RTexture texture;

	char* target;
	char* storage;

	ButtonMouseState mouse_state;
    RSound enter_se;

	int flags;
} ButtonObject;

typedef struct {
    UIObject base;

	RVec2 position;
	RTextInstance text_instance;
} TextObject;

TextObject* create_text(FataState* state, char* text);
ButtonObject* create_button(FataState* state, RTexture texture, char* storage, char* target, int flags);
