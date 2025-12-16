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
    PrintConsole console_top;
    PrintConsole console_bottom;
} Global3DS;

// This is stupid but whatever
static Global3DS global_3ds = {0};


void debug_print_memory(FataState* state) {
    //return;

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
    consoleSelect(&global_3ds.console_top);
}

void r_init(FataState* state) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    consoleInit(GFX_BOTTOM, &global_3ds.console_bottom);
    consoleInit(GFX_TOP, &global_3ds.console_top);
    printf("FataMoru\n"); // Green text
}
void r_shutdown() {
    C2D_Fini();
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

RTexture r_create_render_texture(RVec2 size) {
	return (RTexture) {
		.valid = true,
		.size = size
	};
}


void r_begin_render_texture_draw(RTexture texture) { }
void r_end_render_texture_draw() { }

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
