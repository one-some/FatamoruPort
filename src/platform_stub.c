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

double r_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((tv.tv_sec) * 1000.0d) + (tv.tv_usec / 1000.0d);
}

RTexture r_load_texture(char* path) { }
void r_unload_texture(RTexture texture) { }

RFont r_load_font(char* path) { }
void r_unload_font(RFont font) { }

#endif
