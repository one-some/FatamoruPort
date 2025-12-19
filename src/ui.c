#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "state.h"

TextObject* create_text(FataState* state, char* text) {
	TextObject* text_object = malloc(sizeof(TextObject));

	text_object->base = (UIObject) { .type = VO_TEXT };

	text_object->text_instance = r_create_text(
		text,
		state->active_screen->active_layer->font
	);

	text_object->position = state->active_screen->active_layer->pointer_pos;

	RVec2 size = r_measure_text(text_object->text_instance.font, text);
	state->active_screen->active_layer->pointer_pos.x += size.x;

	printf("Making text on %s\n", state->active_screen->active_layer->name);

	v_append(&state->active_screen->active_layer->children, text_object);

	return text_object;
}

ButtonObject* create_button(FataState* state, RTexture texture, char* storage, char* target, int flags) {
	ButtonObject* button = malloc(sizeof(ButtonObject));
	button->base = (UIObject) { .type = VO_BUTTON };
	button->storage = storage;
	button->target = target;
	button->position = state->active_screen->active_layer->pointer_pos;
	button->mouse_state = BUTTON_MOUSE_NONE;
	button->texture = texture;
	button->flags = flags;

	v_append(&state->active_screen->active_layer->children, button);

	return button;
}
