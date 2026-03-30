#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "state.h"

TextObject* create_text(
    FataState* state,
    char* text,
    VisualLayer* layer
) {
    if (!layer) layer = state->active_screen->active_layer;

	TextObject* text_object = malloc(sizeof(TextObject));
	text_object->base = (UIObject) { .type = VO_TEXT };

	text_object->text_instance = r_create_text(
		text,
		*layer->font
	);

	text_object->position = layer->pointer_pos;

    float scale_x = (float)state->active_screen->size.x / (float)state->canvas_size.x;
	RVec2 size = r_measure_text(text_object->text_instance);
	layer->pointer_pos.x += size.x / scale_x;

    printf(
        "[maketxt] '%s' Layer: %s, At: (%d, %d), Size: (%d, %d)\n",
        text,
        layer->name,
        text_object->position.x,
        text_object->position.y,
        size.x,
        size.y
    );

	v_append(&layer->children, text_object);

	return text_object;
}

ButtonObject* create_button(
    FataState* state,
    RTexture texture,
    char* storage,
    char* target,
    int flags,
    VisualLayer* layer
) {
	ButtonObject* button = malloc(sizeof(ButtonObject));
	button->base = (UIObject) { .type = VO_BUTTON };
	button->storage = storage;
	button->target = target;


    if (!layer) layer = state->active_screen->active_layer;

	button->position = layer->pointer_pos;
	button->mouse_state = BUTTON_MOUSE_NONE;
	button->texture = texture;
	button->flags = flags;

	v_append(&layer->children, button);

	return button;
}
