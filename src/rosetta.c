#include "rosetta.h"

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

