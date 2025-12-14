#include <assert.h>
#include <string.h>

#include "visual.h"
#include "state.h"
#include "ui.h"

void init_layer(FataState* state, VisualLayer* layer, char* name) {
	layer->texture_valid = false;
	layer->texture_offset = (Vec2) { 0, 0 };
	layer->name = name;
	layer->children = v_new();
	layer->pointer_pos = (Vec2) { 0, 0 };

	layer->font = state->visual.default_font;
	layer->margins = (Margins) {
		.left = 70,
		.top = 0,
		.right = 70,
		.bottom = 20
	};
}

void init_page(FataState* state, VisualPage* page) {
	init_layer(state, &page->base_layer, "base");
	init_layer(state, &page->layer_zero, "0");
	init_layer(state, &page->layer_one, "1");
	init_layer(state, &page->layer_two, "2");
	init_layer(state, &page->message_layer_zero, "message0");
	init_layer(state, &page->message_layer_one, "message1");
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

void draw_layer(FataState* state, VisualLayer* layer, Vector2 mouse_pos) {
    if (layer->texture_valid) {
		//printf("Drawing a texture on '%s'\n", layer->name);
		DrawTexture(
			layer->texture,
			layer->texture_offset.x,
			layer->texture_offset.y,
			WHITE
		);
	}

	for (int i=0; i<layer->children.length;i++) {
        UIObject* obj = v_get(&layer->children, i);
        assert(obj);

        if (obj->type == VO_BUTTON) {
            ButtonObject* button = (ButtonObject*)obj;

            int pos_x = button->position.x;
            int pos_y = button->position.y;

            Rectangle rect = (Rectangle) {
                pos_x,
                pos_y,
                button->textures.normal.width,
                button->textures.normal.height
            };

            bool mouse_inside = CheckCollisionPointRec(mouse_pos, rect);

            if (!button->hovered && mouse_inside) {
                play_sound(&button->enter_se);
            }

            button->hovered = mouse_inside;

            DrawTexture(
                mouse_inside ? button->textures.hover : button->textures.normal,
                pos_x,
                pos_y,
                WHITE
            );

            // DrawRectangleLinesEx(rect, 1.0f, RED);

            if (mouse_inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                load(state, NULL, button->target);
            }
        } else if (obj->type == VO_TEXT) {
            TextObject* text_obj = (TextObject*)obj;
			FontConfig* font = &state->visual.active_layer->font;

			DrawTextEx(
				text_obj->font.resource,
				text_obj->text,
				(Vector2) { text_obj->position.x, text_obj->position.y },
				(float)font->size,
                0.0f,
				font->color
			);
        } else {
            assert(false);
        }
	}
}

void draw_page(FataState* state, VisualPage* page, Vector2 mouse_pos) {
	//printf("Drawing Page: '%s'\n", page->name);
    
    draw_layer(state, &page->base_layer, mouse_pos);
    draw_layer(state, &page->layer_zero, mouse_pos);
    draw_layer(state, &page->layer_one, mouse_pos);
    draw_layer(state, &page->layer_two, mouse_pos);

    draw_layer(state, &page->message_layer_zero, mouse_pos);
    draw_layer(state, &page->message_layer_one, mouse_pos);
}


void unload_page_textures(VisualPage* page) {
	// TODO: FIXME!!~!
	return;

    VisualLayer* layers[] = {
        &page->base_layer, &page->layer_zero, &page->layer_one, 
        &page->layer_two, &page->message_layer_zero, &page->message_layer_one
    };

    for (int i = 0; i < 6; i++) {
        if (layers[i]->texture_valid) {
            UnloadTexture(layers[i]->texture);
            layers[i]->texture_valid = false;
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
		page = &state->visual.fore;
	} else if (strcmp("back", page_name) == 0) {
		page = &state->visual.back;
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
}

