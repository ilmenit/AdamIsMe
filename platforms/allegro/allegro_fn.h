#pragma once
#ifndef ALLEGRO_FN_H
#define ALLEGRO_FN_H

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define SCREEN_MAP_POS_X 20
#define SCREEN_MAP_POS_Y 200
#define SCREEN_TILE_SIZE 32

#define TILES_ROWS (4+1)
#define TILES_COLUMNS 16

#define FIRST_DIRECTION_TILE (4 * TILES_COLUMNS)

#define TILES_MAX 64
#define EMPTY_TILE 63

void init_editor();
bool open_level_set();
void show_error(const char *error);

#endif
