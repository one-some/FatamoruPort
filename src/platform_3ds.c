#ifdef PLATFORM_3DS

#include "rosetta.h"
#include <sys/time.h>
#include <stddef.h>

#include <citro2d.h>
#include <3ds.h>

u32 __ctru_linear_heap_size = 32 * 1024 * 1024;

// typedef struct Global3DS {
// 
// };
// 
// // This is stupid but whatever
// static Global3DS global_3ds = {0};

#include <malloc.h> // Required for mallinfo

void debug_print_memory() {
    struct mallinfo info = mallinfo();

    // 1. Total Heap Size (The "124MB" you saw earlier)
    // This is the total memory the OS gave your app.
    u32 total_heap_mem = info.arena; 

    // 2. Actually Used (Textures, Variables, Structs)
    // This is the number you need to watch!
    u32 actually_used_mem = info.uordblks; 

    // 3. True Free Space (Space left for new mallocs)
    u32 true_free_mem = info.fordblks;

    printf("\x1b[0;0H"); // Move cursor to top
    printf("=== REAL Memory Stats ===\n");
    printf("Total Heap:  %6ld KB (%.2f MB)\n", total_heap_mem / 1024, total_heap_mem / 1024.0 / 1024.0);
    printf("Active Data: %6ld KB (%.2f MB)\n", actually_used_mem / 1024, actually_used_mem / 1024.0 / 1024.0);
    printf("True Free:   %6ld KB (%.2f MB)\n", true_free_mem / 1024, true_free_mem / 1024.0 / 1024.0);

    // Warning Logic
    if (true_free_mem < (2 * 1024 * 1024)) { // Less than 2MB free
        printf("\x1b[31m[CRITICAL WARNING] LOW MEMORY!\x1b[0m\n");
    }
}

void r_init(FataState* state) {
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    consoleInit(GFX_BOTTOM, NULL);
    printf("FataMoru\n"); // Green text
}
void r_shutdown() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
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

	debug_print_memory();
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
	return (RTexture) {
		.valid = true
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
