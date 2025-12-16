#ifdef PLATFORM_STUB

#include "rosetta.h"
#include <sys/time.h>
#include <stddef.h>

void r_init(FataState* state) { }
void r_shutdown() { }

bool r_main_loop(FataState* state) {
	return true;
}

void r_begin_frame() { }
void r_end_frame() { }

void r_clear_frame(RColor color) { }

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
