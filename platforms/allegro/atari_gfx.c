#define ALLEGRO_STATICLINK 1
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>
#include "../../common/platform.h"
#include "../../common/extern.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "atari_gfx.h"
#include "allegro_fn.h"

//////////////////////////////////////////////////////
extern ALLEGRO_DISPLAY * display;
extern struct atari_tiles_info_def atari_tiles_info[WORLDS_MAX + 1];

struct rgb atari_palette[128]; // 128 colors 

bool load_atari_palette(const char *filename)
{
	size_t i;
	struct rgb col;
	FILE *fp = fopen(filename, "rb");
	if (!fp)
		return false;
	for (i = 0; i < 256; ++i)
	{
		col.r = fgetc(fp);
		col.g = fgetc(fp);
		col.b = fgetc(fp);
		// limit it to 128 colors!
		// use every second color
		if (i % 2 == 0)
			atari_palette[i >> 1] = col;
	}
	fclose(fp);
	return true;
}

ALLEGRO_COLOR atari_color_to_allegro_color(byte atari_color)
{
	ALLEGRO_COLOR new_color;
	byte r, g, b;
	atari_color /= 2; // we have 128 colors from 256
	r = atari_palette[atari_color].r;
	g = atari_palette[atari_color].g;
	b = atari_palette[atari_color].b;
	new_color = al_map_rgb(r, g, b);
	return new_color;
}

void remap_palette(struct ALLEGRO_BITMAP *indices, struct ALLEGRO_BITMAP *bitmap)
{
	int width, height;
	int x, y;
	byte index1, index2, index3;
	ALLEGRO_LOCKED_REGION *in;
	ALLEGRO_LOCKED_REGION *bt;
	ALLEGRO_COLOR index_color;
	ALLEGRO_COLOR new_color;

	width = al_get_bitmap_width(bitmap);
	height = al_get_bitmap_height(bitmap);

	in = al_lock_bitmap(indices, ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8, ALLEGRO_LOCK_READONLY);
	bt = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);

	al_set_target_bitmap(bitmap);

	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			index_color = al_get_pixel(indices, x, y);
			al_unmap_rgb(index_color, &index1, &index2, &index3);

			assert(index1 < COLORS_MAX);
			if (index1 >= COLORS_MAX)
			{
				show_error("On GFX picture there are more colors than in selectable palette.");
				exit(0);
			}
			byte atari_color = atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[index1];
			new_color = atari_color_to_allegro_color(atari_color);
			al_put_pixel(x, y, new_color);
		}
	}
	al_unlock_bitmap(indices);
	al_unlock_bitmap(bitmap);
	al_set_target_bitmap(al_get_backbuffer(display));
}

void save_tiles_to_font(struct ALLEGRO_BITMAP *indices, char *fnt_name)
{
	int width, height;
	int char_x, char_y;
	int x, y;
	ALLEGRO_LOCKED_REGION *in;
	ALLEGRO_COLOR index_color;
	byte index1, index2, index3;

	width = al_get_bitmap_width(indices);
	height = al_get_bitmap_height(indices);

	if (width != 16 * PNG_TILE_SIZE || height < 4 * PNG_TILE_SIZE)
	{
		show_error("Wrong size of gfx bitmap with tiles");
		exit(1);
	}

	FILE *fp;
	fp = fopen(fnt_name, "wb+");

	in = al_lock_bitmap(indices, ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8, ALLEGRO_LOCK_READONLY);

	// each character in font is 8*8 pixels in input bitmap, but we take every second pixel horizontally 
	const int font_rows = 8;
	const int font_coluns = 32;
	const int char_size = 8;
	for (char_y = 0; char_y < font_rows; ++char_y)
	{
		for (char_x = 0; char_x < font_coluns; ++char_x)
		{
			for (y = 0; y < char_size; ++y)
			{
				byte char_val = 0;
				for (x = 0; x < char_size; x += 2)
				{
					index_color = al_get_pixel(indices, char_x * char_size + x, char_y * char_size + y);
					al_unmap_rgb(index_color, &index1, &index2, &index3);

					if (index1 > 3)
						index1 = 3;

					char_val |= index1 << (6-x);
				}
				fwrite(&char_val, 1, 1, fp);
			}
		}
	}
	fclose(fp);
	al_unlock_bitmap(indices);
}

void save_inverse_information(struct ALLEGRO_BITMAP *indices, char *fnt_name)
{
	int width, height;
	ALLEGRO_LOCKED_REGION *in;
	ALLEGRO_COLOR index_color;
	byte index1, index2, index3;

	width = al_get_bitmap_width(indices);
	height = al_get_bitmap_height(indices);

	if (width != 16 * PNG_TILE_SIZE || height < 4 * PNG_TILE_SIZE)
	{
		show_error("Wrong size of gfx bitmap with tiles");
		exit(1);
	}

	FILE *fp;
	fp = fopen(fnt_name, "wb+");

	in = al_lock_bitmap(indices, ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8, ALLEGRO_LOCK_READONLY);

	// each character in font is 8*8 pixels in input bitmap, but we take every second horizontally
	int char_x, char_y;
	int pixel_x, pixel_y;
	int char_in_tile;
	int tile_index = 0;
	const int tiles_in_row = 16;
	byte invert_array[TILES_MAX];

	for (tile_index = 0; tile_index < TILES_MAX; ++tile_index)
	{
		char_in_tile = 0;
		byte invert_tile_info=0;
		for (char_y = 0; char_y < 2; ++char_y)
		{
			for (char_x = 0; char_x < 2; ++char_x)
			{
				bool char_inverted = false;
				int pos_x = (tile_index % tiles_in_row ) * PNG_TILE_SIZE;
				int pos_y = (tile_index / tiles_in_row ) * PNG_TILE_SIZE;
				for (pixel_y = 0; pixel_y < 8 && (char_inverted==false); ++pixel_y)
				{
					for (pixel_x = 0; pixel_x < 8; pixel_x += 2)
					{
						int x = pos_x + char_x * 8 + pixel_x;
						int y = pos_y + char_y * 8 + pixel_y;
						index_color = al_get_pixel(indices, x, y);
						al_unmap_rgb(index_color, &index1, &index2, &index3);
						if (index1 == 4)
						{
							char_inverted = true;
							break;
						}
					}
				}
				if (char_inverted)
					invert_tile_info |= (1 << char_in_tile);
				++char_in_tile;
			}
		}
		invert_array[tile_index] = invert_tile_info; 
	}
	fwrite(invert_array, 1, TILES_MAX, fp);
	fclose(fp);
	al_unlock_bitmap(indices);
}