#pragma once
#define ALLEGRO_STATICLINK 1
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>
#include "../../common/definitions.h"

#define PNG_TILE_SIZE 16

struct rgb {
	unsigned char b;
	unsigned char g;
	unsigned char r;
};

bool load_atari_palette(const char *filename);
void remap_palette(struct ALLEGRO_BITMAP *indices, struct ALLEGRO_BITMAP *bitmap);
ALLEGRO_COLOR atari_color_to_allegro_color(byte atari_color);
void save_tiles_to_font(struct ALLEGRO_BITMAP *indices, char *fnt_name);
void save_inverse_information(struct ALLEGRO_BITMAP *indices, char *fnt_name);
