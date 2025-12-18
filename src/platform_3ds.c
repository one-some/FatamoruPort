#ifdef PLATFORM_3DS

#include "rosetta.h"
#include "fs.h"
#include "ui.h"
#include <sys/time.h>
#include <stddef.h>

#include <malloc.h>
#include <citro2d.h>
#include <3ds.h>
#include "state.h"
#include "platform_3ds.h"

// u32 __ctru_linear_heap_size = 32 * 1024 * 1024;

// This is stupid but whatever
static Global3DS global_3ds = {0};

void debug_print_memory(FataState* state) {
    return;
    struct mallinfo info = mallinfo();
    u32 total_heap_mem = info.arena; 
    u32 actually_used_mem = info.uordblks; 
    u32 true_free_mem = info.fordblks;
    u32 arena_used = state->static_arena.offset;
    u32 linear_free = linearSpaceFree();

    printf("\x1b[0;0H\e[0;36m");
    printf("=== REAL Memory Stats ===\n");
    printf("Total Heap:  %6ld KB (%.2f MB)\n", total_heap_mem / 1024, total_heap_mem / 1024.0 / 1024.0);
    printf("Active Data: %6ld KB (%.2f MB)\n", actually_used_mem / 1024, actually_used_mem / 1024.0 / 1024.0);
    printf("True Free:   %6ld KB (%.2f MB)\n", true_free_mem / 1024, true_free_mem / 1024.0 / 1024.0);
    printf("Area Usage:   %6ld KB (%.2f MB)\n", arena_used / 1024, arena_used / 1024.0 / 1024.0);
    printf("Linear Free:   %6ld KB (%.2f MB)\n", linear_free / 1024, linear_free / 1024.0 / 1024.0);
    printf("\e[0;0m");
}

uint64_t next_pow2(uint64_t x) {
	// https://jameshfisher.com/2018/03/30/round-up-power-2/
	uint64_t p = 1;
	while (p < x) p <<= 1;
	return p;
}

void r_post_init(FataState* state) {
	init_layer(state, &global_3ds.bottom_layer, "CTR_BOTTOM_LAYER");
}

void r_init(FataState* state) {
	state->window_size = (RVec2) {
		next_pow2(400),
		next_pow2(240)
	};

    romfsInit();
    gfxInitDefault();

    bool success = C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	assert(success);

    success = C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	assert(success);
    C2D_Prepare();

    printf("FataMor2\n");
	global_3ds.special_state = SPECIAL_STATE_NONE;

	global_3ds.top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	assert(global_3ds.top_target);
	global_3ds.bottom_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	assert(global_3ds.bottom_target);

    printf("FataMoru\n");
}
void r_shutdown() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
}

void r_dbgout(char* string, size_t length) {
	__real_printf("%s", string);

	if (string[length - 1] == '\n') string[length - 1] = '\0';
	svcOutputDebugString(string, strlen(string));
}

void r_step(FataState* state) {
	debug_print_memory(state);
}

bool r_main_loop(FataState* state) {
    bool out = aptMainLoop();

    hidScanInput();

	return out;
}

void r_begin_frame(FataState* state) {
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

	if (global_3ds.special_state == SPECIAL_STATE_TITLE) {
		title_frame(state);
	}

	C2D_SceneBegin(global_3ds.top_target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
}
void r_end_frame() {
	C3D_FrameEnd(0);
}

void r_clear_frame(RColor color) {
	C2D_TargetClear(global_3ds.top_target, C2D_Color32(color.r, color.g, color.b, color.a));
}

double r_time_ms() {
    return (double)(svcGetSystemTick() / CPU_TICKS_PER_MSEC);
}

RTexture r_load_texture(char* path) {
    // TODO: Look at the header and see if it's really this serious
    C2D_SpriteSheet* sheet = malloc(sizeof(C2D_SpriteSheet));
    *sheet = C2D_SpriteSheetLoad(path);
    assert(*sheet);

    C2D_Image* image = malloc(sizeof(C2D_Image));
    *image = C2D_SpriteSheetGetImage(*sheet, 0);

	return (RTexture) {
		.valid = true,
        .resource = image
	};
}

void r_unload_texture(RTexture texture) { }

void r_draw_texture_tint_sample(RTexture texture, RVec2 position, RColor tint, RRect sample_rect) {
    // TODO: Everything else
    assert(texture.valid);

    // printf("Drawing Texture %d (%d, %d)\n", ((int)texture.resource & 0xFFF), position.x, position.y);

    C2D_Image* image = (C2D_Image*)texture.resource;
	C2D_DrawImageAt(
        *image,
        position.x,
        position.y,
        0.5f,
        NULL,
        1.0f,
        1.0f
    );
}


RRenderTexture r_create_render_texture(RVec2 size) {
    printf("[rendtex] Alloc %d x %d\n", size.x, size.y);

    CtrRenderTargetBundle* bundle = calloc(1, sizeof(CtrRenderTargetBundle));
	assert(bundle);

    bool vram_success = C3D_TexInitVRAM(&bundle->texture, size.x, size.y, GPU_RGBA8);
	assert(vram_success);

    bundle->target = C3D_RenderTargetCreateFromTex(
        &bundle->texture,
        GPU_TEXFACE_2D,
        0,
        GPU_RB_DEPTH24_STENCIL8
    );

	assert(bundle->target);

	Tex3DS_SubTexture* subtex = malloc(sizeof(Tex3DS_SubTexture));
	subtex->width = size.x;
	subtex->height = size.y;
	subtex->left = 0.0f;
	subtex->top = 1.0f;
	subtex->right = 1.0f;
	subtex->bottom = 0.0f;

	bundle->image.tex = &bundle->texture;
	bundle->image.subtex = subtex;

    return (RRenderTexture) {
        .valid = true,
        .size = size,
        .resource = bundle
    };
}

void r_draw_render_texture(RRenderTexture texture, float alpha) {
    CtrRenderTargetBundle* bundle = (CtrRenderTargetBundle*)texture.resource;
    assert(bundle);

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, alpha);

	C2D_DrawImageAt(bundle->image, 0, 0, 0.5f, &tint, 1.0f, 1.0f);
}


void r_begin_render_texture_draw(RRenderTexture texture) {
	CtrRenderTargetBundle* bundle = (CtrRenderTargetBundle*)texture.resource;
	assert(bundle);

	C2D_TargetClear(bundle->target, C2D_Color32(0xFF, 0, 0, 0xFF));
	C2D_SceneBegin(bundle->target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
}

void r_end_render_texture_draw(RRenderTexture texture) {
	CtrRenderTargetBundle* bundle = (CtrRenderTargetBundle*)texture.resource;
	assert(bundle);

	C2D_SceneBegin(global_3ds.top_target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
}

RFont r_load_font(char* path) {
	printf("[dbg] Loading font '%s'\n", path);

	// Actually a ptr....
	C2D_Font font = C2D_FontLoad(path);
	assert(font);

	return (RFont) {
		.resource = font
	};
}
void r_unload_font(RFont font) { }

RTextInstance r_create_text(char* string, RFont font) {
	CtrTextBundle* bundle = malloc(sizeof(CtrTextBundle));
	assert(bundle);

	bundle->text_buf = C2D_TextBufNew(strlen(string));
	assert(bundle->text_buf);

	C2D_Font c_font = (C2D_Font)font.resource;
	assert(c_font);

	//C2D_TextParse(&bundle->text, bundle->text_buf, string);
	C2D_TextFontParse(&bundle->text, c_font, bundle->text_buf, string);
	C2D_TextOptimize(&bundle->text);

	return (RTextInstance) {
		.resource = bundle
	};
}

void r_draw_text(RTextInstance text_instance, RVec2 position) {
	CtrTextBundle* bundle = (CtrTextBundle*)text_instance.resource;
	assert(bundle);

	assert(&bundle->text);

	float size = 0.8;
	C2D_DrawText(
		&bundle->text,
		C2D_WithColor,
		position.x,
		position.y,
		0.2f,
		size,
		size,
		C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f)
	);
}

RVec2 r_measure_text(RFont font, char* text) {
	return (RVec2) { 0, 0 };
}

RSound r_load_sound(char* path) {
	return (RSound) {
		.valid = true
	};
}

void r_play_sound(RSound track) { }
void r_stop_sound(RSound track) { }
void r_unload_sound(RSound track) { }

bool r_get_click() {
    u32 down = hidKeysDown();
    if (!(down & KEY_TOUCH)) return false;

    touchPosition pos;
    hidTouchRead(&pos);
    printf("T: (%d, %d)\n", pos.px, pos.py);

	return true;
}

bool r_get_skip_held() {
	return false;
}

RVec2 r_get_cursor_pos() {
	return (RVec2) { 0, 0 };
}

void r_set_window_title(char* title) { }

void title_hook(FataState* state) {
	printf("HELLOTITLEWORLD");

	assert(!global_3ds.bottom_layer.texture.valid);
	global_3ds.special_state = SPECIAL_STATE_TITLE;

	state->visual.fore.base_layer.texture = r_load_texture(DATA_PATH("bgimage/オープニング.t3x"));


	// Fata BG
	VisualLayer* old_layer = state->visual.active_layer;

	state->visual.active_layer = &global_3ds.bottom_layer;
	global_3ds.bottom_layer.texture = r_load_texture(DATA_PATH("bgimage/massageback.t3x"));
    global_3ds.overlay = r_load_texture(PATH("static/mockup.t3x"));

    // global_3ds.bottom_layer.pointer_pos = (RVec2) { 50, 50 };
	// create_text(state, "Enter the Mansion");
	// create_text(state, "Inspect Your Memories");
    // create_text(state, "Config");
    // create_text(state, "Extras");


	state->visual.active_layer = old_layer;

	// if (false) {
	// 	global_3ds.special_state = SPECIAL_STATE_NONE;
	// 	jump_to_point(state, DATA_PATH("scenario/scenario.ks"), "start");
	// }
}

void title_frame(FataState* state) {
	assert(global_3ds.bottom_layer.texture.valid);

	C2D_SceneBegin(global_3ds.bottom_target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);

	C2D_Fade(C2D_Color32f(0.0f, 0.0f, 0.0f, 0.5f));
	draw_layer(state, &global_3ds.bottom_layer, DRAW_TEXTURES);
	C2D_Fade(0);
	draw_layer(state, &global_3ds.bottom_layer, DRAW_CHILDREN);

    r_draw_texture(global_3ds.overlay, (RVec2) { 0, 0 });

	// RTextInstance ti = r_create_text("L0L FROM Fata Morgana", (RFont) { 0 });
	// r_draw_text(ti, (RVec2) { 0, 0 });
}

bool r_jump_hook(FataState* state, char* storage) {
	if (strcmp(storage, "title.ks") == 0) {
		// We have a special menu for the 3ds!
		state->stopped = true;
		title_hook(state);

		return true;
	}

	return false;
}

#endif
