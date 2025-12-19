#ifdef PLATFORM_3DS

#include "rosetta.h"
#include "visual.h"
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
	// init_layer(state, &global_3ds.bottom_layer, "CTR_BOTTOM_LAYER");
}

void r_init(FataState* state) {
	init_screen(&state->primary_screen_storage, "primary", (RVec2) { 400, 240 });
	init_screen(&global_3ds.bottom_screen, "bottom", (RVec2) { 320, 240 });

    romfsInit();
    gfxInitDefault();

    bool success = C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	assert(success);

    success = C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	assert(success);
    C2D_Prepare();

    printf("FataMor2\n");

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

	C2D_SceneBegin(global_3ds.top_target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
}
void r_end_frame(FataState* state) {
	C2D_SceneBegin(global_3ds.bottom_target);
    draw_screen(state, &global_3ds.bottom_screen);

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
        .resource = image,
		.size = (RVec2) {
			image->subtex->width,
			image->subtex->height
		}
	};
}

void r_unload_texture(RTexture texture) { }

void r_draw_texture_tint_sample(RTexture texture, RVec2 position, RColor tint, RRect sample_rect) {
    // TODO: Everything else
    assert(texture.valid);

    // printf("Drawing Texture %d (%d, %d)\n", ((int)texture.resource & 0xFFF), position.x, position.y);

    C2D_ImageTint c_tint;
	C2D_PlainImageTint(
		&c_tint,
		C2D_Color32(tint.r, tint.g, tint.b, 0xFF),
		((float)tint.a) / 255.0f
	);

    C2D_Image* image = (C2D_Image*)texture.resource;
	C2D_DrawImageAt(
        *image,
        position.x,
        position.y,
        0.5f,
        &c_tint,
        1.0f,
        1.0f
    );
}


RRenderTexture r_create_render_texture(RVec2 size) {
    printf("[rendtex] Alloc %d x %d\n", size.x, size.y);
    size.x = next_pow2(size.x);
    size.y = next_pow2(size.y);
    printf("[rendtex] Up! -> %d x %d\n", size.x, size.y);

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


void r_begin_render_texture_draw(VisualScreen* screen, RRenderTexture texture) {
	CtrRenderTargetBundle* bundle = (CtrRenderTargetBundle*)texture.resource;
	assert(bundle);

	C2D_TargetClear(bundle->target, C2D_Color32(0xFF, 0, 0, 0xFF));
	C2D_SceneBegin(bundle->target);
	C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
}

C3D_RenderTarget* screen_to_render_target(VisualScreen* screen) {
    bool is_bottom = screen == &global_3ds.bottom_screen;
    return is_bottom ? global_3ds.bottom_target : global_3ds.top_target;
}

void r_end_render_texture_draw(VisualScreen* screen, RRenderTexture texture) {
	CtrRenderTargetBundle* bundle = (CtrRenderTargetBundle*)texture.resource;
	assert(bundle);

    C3D_RenderTarget* target = screen_to_render_target(screen);

	C2D_SceneBegin(target);
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
	return down & KEY_TOUCH;
}

bool r_get_click_down() {
    u32 down = hidKeysHeld();
	return down & KEY_TOUCH;
}

bool r_get_skip_held() {
	return false;
}

RVec2 r_get_cursor_pos() {
    touchPosition pos;
    hidTouchRead(&pos);

	static touchPosition last_pos = { 0 };
	if (pos.px || pos.py) {
		last_pos = pos;
	}

	return (RVec2) { last_pos.px, last_pos.py };
}

void r_set_window_title(char* title) { }

char* r_jump_hook(FataState* state, char* script_name) {
	if (strcmp(script_name, "title.ks") == 0) {
		return "3ds_title.ks";
	}

	return NULL;
}

void swap_screens(FataState* state) {
    bool is_top = state->active_screen == &state->primary_screen_storage;

    state->active_screen = is_top ? &global_3ds.bottom_screen : &state->primary_screen_storage;

	printf("SWAP!!\n");
}

bool r_command_hook(FataState* state, char* cmd, Vector* args) {
	if (strcmp("claire_ctrswapscreen", cmd) == 0) {
		swap_screens(state);
		return true;
	}

	return false;
}

#endif
