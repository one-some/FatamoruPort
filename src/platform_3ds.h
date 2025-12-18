#pragma once

#include "visual.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct {
	C3D_Tex texture;
	C3D_RenderTarget* target;
	C2D_Image image;
} CtrRenderTargetBundle;

typedef struct {
	C2D_TextBuf text_buf;
	C2D_Text text;
} CtrTextBundle;

typedef enum {
	SPECIAL_STATE_NONE,
	SPECIAL_STATE_TITLE
} SpecialState;

typedef struct {
	VisualScreen other;
	bool bottom_active;
} ScreenSwapper;

typedef struct {
	C3D_RenderTarget* top_target;
	C3D_RenderTarget* bottom_target;
	SpecialState special_state;

	ScreenSwapper dual;

    RTexture overlay;
} Global3DS;

void debug_print_memory(FataState* state);
uint64_t next_pow2(uint64_t x);
void title_hook(FataState* state);
void title_frame(FataState* state);
void swap_screens(FataState* state);
