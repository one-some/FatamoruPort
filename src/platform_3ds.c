#ifdef PLATFORM_3DS

#include "rosetta.h"
#include "state.h"
#include <sys/time.h>
#include <stddef.h>

#include <malloc.h>
#include <citro2d.h>
#include <3ds.h>

// u32 __ctru_linear_heap_size = 32 * 1024 * 1024;

typedef struct {
	C3D_Tex texture;
	C3D_RenderTarget* target;
	C2D_Image image;
} CitroRenderTargetBundle;

typedef struct {
    PrintConsole console_top;
    PrintConsole console_bottom;
	C3D_RenderTarget* top_target;
} Global3DS;

// This is stupid but whatever
static Global3DS global_3ds = {0};


void debug_print_memory(FataState* state) {
    struct mallinfo info = mallinfo();
    u32 total_heap_mem = info.arena; 
    u32 actually_used_mem = info.uordblks; 
    u32 true_free_mem = info.fordblks;
    u32 arena_used = state->static_arena.offset;
    u32 linear_free = linearSpaceFree();

    consoleSelect(&global_3ds.console_bottom);
    printf("\x1b[0;0H\e[0;36m");
    printf("=== REAL Memory Stats ===\n");
    printf("Total Heap:  %6ld KB (%.2f MB)\n", total_heap_mem / 1024, total_heap_mem / 1024.0 / 1024.0);
    printf("Active Data: %6ld KB (%.2f MB)\n", actually_used_mem / 1024, actually_used_mem / 1024.0 / 1024.0);
    printf("True Free:   %6ld KB (%.2f MB)\n", true_free_mem / 1024, true_free_mem / 1024.0 / 1024.0);
    printf("Area Usage:   %6ld KB (%.2f MB)\n", arena_used / 1024, arena_used / 1024.0 / 1024.0);
    printf("Linear Free:   %6ld KB (%.2f MB)\n", linear_free / 1024, linear_free / 1024.0 / 1024.0);
    printf("\e[0;0m");
    //consoleSelect(&global_3ds.console_top);
}

uint64_t next_pow2(uint64_t x) {
	// https://jameshfisher.com/2018/03/30/round-up-power-2/
	uint64_t p = 1;
	while (p < x) p <<= 1;
	return p;
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

    // success = C2D_Init(64);
	// assert(success);

    // C2D_Prepare();

    consoleInit(GFX_BOTTOM, &global_3ds.console_bottom);


    printf("FataMor2\n"); // Green text

	global_3ds.top_target = C3D_RenderTargetCreate(
		240,
		400,
		GPU_RB_RGBA8,
		GPU_RB_DEPTH24_STENCIL8
	);
	assert(global_3ds.top_target);

	C3D_RenderTargetSetOutput(
		global_3ds.top_target,
		GFX_TOP,
		GFX_LEFT,
		(
			GX_TRANSFER_FLIP_VERT(0) | \
			GX_TRANSFER_OUT_TILED(0) | \
			GX_TRANSFER_RAW_COPY(0) | \
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
			GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)
		)
	);

    printf("FataMoru\n");
}
void r_shutdown() {
    // C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
}

void r_step(FataState* state) {
	debug_print_memory(state);
}

bool r_main_loop(FataState* state) {
	return aptMainLoop();
}

void r_begin_frame() {
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	//C2D_SceneBegin(top);
}
void r_end_frame() {
	C3D_FrameEnd(0);
}

void r_clear_frame(RColor color) {
	//C2D_TargetClear(top, C2D_Color32f(0.2f, 0.2f, 0.2f, 1.0f));
}

double r_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

RTexture r_load_texture(char* path) {
    // TODO: Can we just render an image? Prob would be awesomer
    // TODO: Look at the header and see if it's really this serious
    C2D_SpriteSheet* sheet = malloc(sizeof(C2D_SpriteSheet));
    *sheet = C2D_SpriteSheetLoad(path);
    assert(*sheet);

    C2D_Sprite* sprite = malloc(sizeof(C2D_Sprite));
    C2D_SpriteFromSheet(sprite, *sheet, 0);

	return (RTexture) {
		.valid = true,
        .resource = sprite
	};
}

void r_unload_texture(RTexture texture) { }

RRenderTexture r_create_render_texture(RVec2 size) {
    printf("[rendtex] Alloc %d x %d\n", size.x, size.y);

    CitroRenderTargetBundle* bundle = calloc(1, sizeof(CitroRenderTargetBundle));
	assert(bundle);

    // 2. Try VRAM Allocation
	//C3D_Tex tex;
    bool vram_success = C3D_TexInitVRAM(&bundle->texture, size.x, size.y, GPU_RGBA8);
	assert(vram_success);
	printf("Why are we kosher..?\n");

    // 3. Try Target Creation
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
    CitroRenderTargetBundle* bundle = (CitroRenderTargetBundle*)texture.resource;
	C2D_DrawImageAt(bundle->image, 0, 0, 0.5f, NULL, 1.0f, 1.0f);
}


void r_begin_render_texture_draw(RRenderTexture texture) {
	CitroRenderTargetBundle* bundle = (CitroRenderTargetBundle*)texture.resource;
	assert(bundle);

	C3D_FrameDrawOn(bundle->target);
}

void r_end_render_texture_draw(RRenderTexture texture) {
	CitroRenderTargetBundle* bundle = (CitroRenderTargetBundle*)texture.resource;
	assert(bundle);

	C3D_FrameDrawOn(global_3ds.top_target);
	C3D_TexBind(0, &bundle->texture);
}

void r_draw_texture_tint_sample(RTexture texture, RVec2 position, RColor tint, RRect sample_rect) { }

RFont r_load_font(char* path) { }
void r_unload_font(RFont font) { }
void r_draw_text(RFont font, char* text, RVec2 position) { }
RVec2 r_measure_text(RFont font, char* text) { }

RSound r_load_sound(char* path) {
	return (RSound) {
		.valid = true
	};
}

void r_play_sound(RSound track) { }
void r_stop_sound(RSound track) { }
void r_unload_sound(RSound track) { }

bool r_get_click() {
	return false;
}

bool r_get_skip_held() {
	return true;
}

RVec2 r_get_cursor_pos() {
	return (RVec2) { 0, 0 };
}

void r_set_window_title(char* title) { }

#endif
