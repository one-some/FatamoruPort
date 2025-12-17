#include "rosetta.h"

#include <stdio.h>
#include <stdarg.h>

int __wrap_printf(const char* format, ...) {
    char buffer[512];
    va_list args;

    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

	if (length <= 0) return 0;

	r_dbgout(buffer, length);
	return length;
}

// This kinda suckss
void r_draw_texture(RTexture texture, RVec2 position) {
	r_draw_texture_tint(texture, position, R_WHITE);
}

void r_draw_texture_tint(RTexture texture, RVec2 position, RColor tint) {
	r_draw_texture_tint_sample(
		texture,
		position,
		tint,
		(RRect) { 0, 0, texture.size.x, texture.size.y }
	);
}

