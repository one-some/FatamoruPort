#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "state.h"

ButtonTextureCollection read_button_textures(char* path) {
	ButtonTextureCollection textures;

	Image img_base = LoadImage(path);

	int width = img_base.width / 3;

	Image working_img = ImageFromImage(
		img_base,
		(Rectangle) { 0, 0, width, img_base.height }
	);
	textures.normal = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	working_img = ImageFromImage(
		img_base,
		(Rectangle) { width, 0, width, img_base.height }
	);
	textures.pressed = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	working_img = ImageFromImage(
		img_base,
		(Rectangle) { width * 2, 0, width, img_base.height }
	);
	textures.hover = LoadTextureFromImage(working_img);
	UnloadImage(working_img);

	return textures;
}

void* create_text(FataState* state, char* text) {
	TextObject* text_object = malloc(sizeof(TextObject));
	text_object->base = (UIObject) { .type = VO_TEXT };
	text_object->text = text;
	text_object->font = state->visual.active_layer->font;
	text_object->position = state->visual.active_layer->pointer_pos;

	Vector2 size = MeasureTextEx(
		text_object->font.resource,
		text,
		(float)text_object->font.size,
        0.0f
	);

	state->visual.active_layer->pointer_pos.x += size.x;

	printf("Making text on %s\n", state->visual.active_layer->name);

	v_append(&state->visual.active_layer->children, text_object);
}

