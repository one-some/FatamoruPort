#include <assert.h>
#include <string.h>

#include "visual.h"
#include "types.h"
#include "state.h"
#include "ui.h"

void init_layer(VisualScreen* screen, VisualLayer* layer, char* name) {
	layer->texture.valid = false;
	layer->texture_offset = (RVec2) { 0, 0 };
	layer->name = name;
	layer->children = v_new();
	layer->pointer_pos = (RVec2) { 0, 0 };

	layer->font = screen->default_font;
	layer->margins = (Margins) {
		.left = 70,
		.top = 0,
		.right = 70
		.bottom = 20
	};
}

void init_page(VisualScreen* screen, VisualPage* page) {
    if (!page->render_target.valid) {
        page->render_target = r_create_render_texture(screen->size);
    }

	init_layer(screen, &page->base_layer, "base");
	init_layer(screen, &page->layer_zero, "0");
	init_layer(screen, &page->layer_one, "1");
	init_layer(screen, &page->layer_two, "2");
	init_layer(screen, &page->message_layer_zero, "message0");
	init_layer(screen, &page->message_layer_one, "message1");
}

void init_screen(FataState* state, VisualScreen* screen, char* name, RVec2 size) {
    screen->name = name;
	screen->size = size;

	screen->active_layer = &screen->fore.message_layer_zero;

	screen->fore.name = "fore";
	screen->back.name = "back";
	init_page(screen, &screen->fore);
	init_page(screen, &screen->back);

    screen->valid = true;

	v_append(&state->screens, screen);
}


void copy_layer(VisualLayer* dest, VisualLayer* src) {
	// Blehhhh....... :/

	Margins margin_keep = dest->margins;

	*dest = *src;

	dest->margins = margin_keep;
}

void copy_page(VisualPage* dest, VisualPage* src) {
	copy_layer(&dest->base_layer, &src->base_layer);
    copy_layer(&dest->layer_zero, &src->layer_zero);
    copy_layer(&dest->layer_one, &src->layer_one);
    copy_layer(&dest->layer_two, &src->layer_two);
    copy_layer(&dest->message_layer_zero, &src->message_layer_zero);
    copy_layer(&dest->message_layer_one, &src->message_layer_one);
}

void draw_layer(
    FataState* state,
    VisualLayer* layer,
    int flags
) {
    if (flags & DRAW_TEXTURES && layer->texture.valid) {
		//printf("Drawing a texture on '%s'\n", layer->name);
		r_draw_texture(layer->texture, layer->texture_offset);
	}

    if (flags & DRAW_CHILDREN) {
        for (int i=0; i<layer->children.length;i++) {
            UIObject* obj = v_get(&layer->children, i);
            assert(obj);

            if (obj->type == VO_BUTTON) {
                ButtonObject* button = (ButtonObject*)obj;
                RRect rect = {
                    .x = button->position.x,
                    .y = button->position.y,
                    .width = button->texture.size.x,
                    .height = button->texture.size.y
                };

				printf("Button: (%d, %d)\n", button->position.x, button->position.y);

				bool mouse_down = r_get_click_down();
                RVec2 mouse_pos = r_get_cursor_pos();
                bool mouse_inside = rect_contains(rect, mouse_pos);
				bool traveling = false;

				// printf("Draw MousePos: (%d, %d)\n", mouse_pos.x, mouse_pos.y);
				printf("State: %d, Inside: %d\n", button->mouse_state, mouse_inside);

				if (
					button->mouse_state == BUTTON_MOUSE_NONE &&
					mouse_inside
				) {
					// Not hover -> hover
					button->mouse_state = BUTTON_MOUSE_HOVER;

					printf("Hover start.\n");

                    r_play_sound(button->enter_se);
				} else if (
					button->mouse_state == BUTTON_MOUSE_HOVER &&
					mouse_down
				) {
					// Hover -> depressed
					button->mouse_state = BUTTON_MOUSE_DEPRESSED;
					printf("Depressed\n");

				} else if (
					button->mouse_state == BUTTON_MOUSE_DEPRESSED &&
					!mouse_down
				) {
					// depressed -> maybe click!
					button->mouse_state = BUTTON_MOUSE_NONE;
					traveling = mouse_inside;
					printf("CLICK? %d\n", traveling);
				} else if (
					button->mouse_state == BUTTON_MOUSE_HOVER &&
					!mouse_inside
				) {
					// Hover -> not hover
					button->mouse_state = BUTTON_MOUSE_NONE;
					printf("Hover end\n");
				}

                // Normal, pressed, hovered
                int offset_width = button->texture.size.x / 3;
                int x_offset = mouse_inside ? offset_width * 2 : 0;

				RColor tint = R_BLANK;

				if (button->flags & BUTTON_NATIVE) {
					if (button->flags & BUTTON_DISABLED) {
						tint = (RColor) { 0, 0, 0, 0xCC };
					} else if (
						button->mouse_state == BUTTON_MOUSE_DEPRESSED ||
						button->mouse_state == BUTTON_MOUSE_HOVER
					) {
						tint = (RColor) { 0, 0, 0, 0x88 };
					}

				}

                r_draw_texture_tint(button->texture, button->position, tint);

                if (traveling) {
                    jump_to_point(state, button->storage, button->target);
                }
            } else if (obj->type == VO_TEXT) {
                TextObject* text_obj = (TextObject*)obj;
                RFont* font = &state->active_screen->active_layer->font;

                r_draw_text(
                    text_obj->text_instance,
                    text_obj->position
                );
            } else {
                assert(false);
            }
        }
    }
}

void draw_page(FataState* state, VisualPage* page) {
	// printf("Drawing Page: '%s'\n", page->name);
    
    draw_layer(state, &page->base_layer, DRAW_ALL);
    draw_layer(state, &page->layer_zero, DRAW_ALL);
    draw_layer(state, &page->layer_one, DRAW_ALL);
    draw_layer(state, &page->layer_two, DRAW_ALL);

    draw_layer(state, &page->message_layer_zero, DRAW_ALL);
    draw_layer(state, &page->message_layer_one, DRAW_ALL);
}


void unload_page_textures(VisualPage* page) {
	// TODO: FIXME!!~!
	return;

    VisualLayer* layers[] = {
        &page->base_layer, &page->layer_zero, &page->layer_one, 
        &page->layer_two, &page->message_layer_zero, &page->message_layer_one
    };

    for (int i = 0; i < 6; i++) {
        if (layers[i]->texture.valid) {
            r_unload_texture(layers[i]->texture);
            layers[i]->texture.valid = false;
        }
    }
}

VisualLayer* get_layer(FataState* state, char* layer_name, char* page_name) {
	assert(layer_name);

	// HACK: OKAY SO LETS JUST LIE THEN I GUESS...
	if (!page_name) page_name = "fore";
	assert(page_name);

	VisualPage* page = NULL;
	if (strcmp("fore", page_name) == 0) {
		page = &state->active_screen->fore;
	} else if (strcmp("back", page_name) == 0) {
		page = &state->active_screen->back;
	}
	assert(page);

    VisualLayer* layers[] = {
        &page->base_layer, &page->layer_zero, &page->layer_one, 
        &page->layer_two, &page->message_layer_zero, &page->message_layer_one, NULL
    };

	for (int i=0; layers[i]; i++) {
		if (strcmp(layers[i]->name, layer_name) != 0) continue;
		return layers[i];
	}

	assert(false);
	return NULL;
}

void draw_screen(FataState* state, VisualScreen* screen) {
    // FIXME: Will canvas_size cause a bug with abs-positioned bottom screen stuff? per-screen canvas size pls
	RRect render_rect = {0};
	render_rect.height = (float)screen->size.y;
	render_rect.width = render_rect.height / (float)state->canvas_size.y * (float)state->canvas_size.x;
	render_rect.x = (float)((screen->size.x - render_rect.width) / 2);
	render_rect.y = 0;

    // TODO: Trans stuff based on screen not state
	float fore_to_back_fade = 0.0;
	if (state->transition_max_ms > 0.0f) {
		printf("TMax: %f ... TRem: %f\n", state->transition_max_ms, state->transition_remaining_ms);
		float trans_progress_ms = state->transition_max_ms - state->transition_remaining_ms;
		fore_to_back_fade = trans_progress_ms / state->transition_max_ms;
	}


    // FIXME: EVERYTHING ABOVE THIS HAS GOT TO MOVE

    r_begin_render_texture_draw(screen, screen->fore.render_target);
        // BeginBlendMode(BLEND_CUSTOM_SEPARATE);
        draw_page(state, &screen->fore);
    r_end_render_texture_draw(screen, screen->fore.render_target);

    if (fore_to_back_fade > 0.0f) {
        r_begin_render_texture_draw(screen, screen->back.render_target);
            // BeginBlendMode(BLEND_CUSTOM_SEPARATE);
            draw_page(state, &screen->back);
        r_end_render_texture_draw(screen, screen->back.render_target);
    }

    // r_clear_frame(R_WHITE);

    if (fore_to_back_fade > 0.0f) {
        r_draw_render_texture(screen->back.render_target, 1.0);
    }

    r_draw_render_texture(screen->fore.render_target, 1.0 - fore_to_back_fade);
}

void draw_everything(FataState* state, VisualScreen* screen) {
	r_begin_frame(state);
        draw_screen(state, screen);
	r_end_frame(state);
}

