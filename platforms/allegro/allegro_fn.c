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
#include "allegro_fn.h"
#include "atari_gfx.h"
#include <stdio.h>
#include <sys/types.h>
#include <io.h>

#pragma pack(1)

#if EDITOR_ATARI || PLATFORM_ATARI
struct atari_tiles_info_def atari_tiles_info[WORLDS_MAX + 1];
FILE* atari_tiles_info_file_pointer;
#endif

//////////////////////////////////////////////////////
FILE* level_set_file_pointer;
const char *level_set_name;

// just one character
#define REPRESENTATION_SINGLE     0x0
// four next characters, according to direction
#define REPRESENTATION_DIRECTIONS 0x1
#define REPRESENTATION_ANIMATED   0x2

bool editor_active = false;

struct level_set_header_def level_set_header;

byte representation_obj[TYPE_MAX] = {
	36, // adam
	35, // ship
	54, // wall
	57, // box
	62, // void
	60, // key
	59, // door
	56, // bolt
	61, // bush
	50, // bat // !!!!
	55, // bomb
	48, // bang
	52, // imp
	58, // rock
	44, // magnet
	0xFF // text
};

byte representation_type[TYPE_MAX] = {
	REPRESENTATION_DIRECTIONS | REPRESENTATION_ANIMATED,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE | REPRESENTATION_ANIMATED,
	REPRESENTATION_SINGLE,
	REPRESENTATION_SINGLE | REPRESENTATION_ANIMATED,
	REPRESENTATION_SINGLE | REPRESENTATION_ANIMATED,
	REPRESENTATION_SINGLE,
	REPRESENTATION_DIRECTIONS,
	REPRESENTATION_SINGLE
};

byte representation_text[] = {
	// objects
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,

	// properties
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,

	// operators
	32, 33, 34
};

byte representation_galaxy[] = {
	// worlds (8)
	22, 23, 24, 25, 26, 27, 28, 29,

	// walls (8)
	54, 55, 56, 57, 58, 59, 60, 61,

	// level numbers (8)
	7, 8, 9, 10, 11, 12, 13, 14,

	// locks (8)
	38, 39, 40, 41, 42, 43, 44, 45,

	// backgrounds (12)
	32, 33, 34, 35, 36, 37, 
	48, 49, 50, 51, 52, 53,

	// shuttle  + shuttle landed 
	5, 62,
	// exit unlocked
	4,
	// exit lock
	46
};

//////////////////////////////////////////////////////
ALLEGRO_DISPLAY * display;
ALLEGRO_FONT * font;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_TIMER *game_clock;
ALLEGRO_TIMER *timer;
ALLEGRO_EVENT event;

ALLEGRO_BITMAP *tiles_atlas[TILESET_MAX];
ALLEGRO_BITMAP *tiles_atlas_indices[TILESET_MAX];

ALLEGRO_BITMAP *current_tiles_atlas;

ALLEGRO_BITMAP *tiles[TILES_COLUMNS * TILES_ROWS];
const char *dialog_tile = "Adam Is Me";

/// Required functions by game engine

void audio_music(unsigned char music_id)
{
}

void audio_sfx(unsigned char sfx_id)
{

}

void load_level_data()
{
	long offset = sizeof(struct level_set_header_def);
	offset += (long)level_number * ((long) sizeof(map));
	fseek(level_set_file_pointer, offset, SEEK_SET);
	fread(map, sizeof(map), 1, level_set_file_pointer);
}

void set_tileset()
{
	current_tiles_atlas = tiles_atlas[atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number];

	for (int i = 0; i < TILES_COLUMNS * TILES_ROWS; ++i)
	{
		// remove old bitmaps
		if (tiles[i] != NULL)
		{
			al_destroy_bitmap(tiles[i]);
			tiles[i] = NULL;
		}
		// create new bitmap
		tiles[i] = al_create_bitmap(SCREEN_TILE_SIZE, SCREEN_TILE_SIZE);

		al_set_target_bitmap(tiles[i]);

		int tile_x = i % TILES_COLUMNS;
		int tile_y = i / TILES_COLUMNS;
		int bitmap_x = tile_x * PNG_TILE_SIZE;
		int bitmap_y = tile_y * PNG_TILE_SIZE;

		al_draw_scaled_bitmap(current_tiles_atlas,
			bitmap_x, bitmap_y, PNG_TILE_SIZE, PNG_TILE_SIZE,
			0, 0, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, 0);
	}
	al_set_target_bitmap(al_get_backbuffer(display));
}

void draw_tile(int map_x, int map_y, int tile_id)
{
	int dest_x = SCREEN_MAP_POS_X + map_x * SCREEN_TILE_SIZE;
	int dest_y = SCREEN_MAP_POS_Y + map_y * SCREEN_TILE_SIZE;
	if (tile_id < _countof(tiles))
		al_draw_bitmap(tiles[tile_id], dest_x, dest_y, 0);
}

void draw_direction(int map_x, int map_y, int dir)
{
	int dest_x = SCREEN_MAP_POS_X + map_x * SCREEN_TILE_SIZE + SCREEN_TILE_SIZE / 2;
	int dest_y = SCREEN_MAP_POS_Y + map_y * SCREEN_TILE_SIZE + SCREEN_TILE_SIZE / 2;
	al_draw_scaled_bitmap(tiles[FIRST_DIRECTION_TILE + dir], 0, 0, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE, dest_x, dest_y, SCREEN_TILE_SIZE / 2, SCREEN_TILE_SIZE / 2, 0);
}

byte get_representation(byte obj, byte index)
{
	byte representation;
	byte type;
	if (obj == TYPE_TEXT)
	{
		type = objects.text_type[index];
		representation = representation_text[type];
	}
	else
		representation = representation_obj[obj];
	return representation;
}

void galaxy_draw_screen()
{
#if (!EDITOR_ENABLED)
	al_clear_to_color(al_map_rgb(20, 20, 40));
#endif
	// draw border over the playfield
	al_draw_filled_rounded_rectangle(SCREEN_MAP_POS_X - 10, SCREEN_MAP_POS_Y - 10,
		SCREEN_MAP_POS_X + MAP_SIZE_X * SCREEN_TILE_SIZE + 10, SCREEN_MAP_POS_Y + MAP_SIZE_Y * SCREEN_TILE_SIZE + 10,
		10, 10,
		atari_color_to_allegro_color(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[0]));

	// draw background
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			MapGet(local_x, local_y, local_type);
			if (local_type == LEVEL_DECODE_EMPTY)
				draw_tile(local_x, local_y, local_type);
			else if (local_type >= DECODE_LEVEL_NUMBERS_MIN && local_type < DECODE_BACKGROUND_MAX)
			{
				draw_tile(local_x, local_y, representation_galaxy[local_type]);
			}
			else
				draw_tile(local_x, local_y, representation_galaxy[local_type]);
		}
	}

	// draw collected nuts
	if (!editor_active)
	{
		if (game_progress.completed_levels > 0)
		{
			byte lo = game_progress.completed_levels % 10;
			draw_tile(MAP_SIZE_X - 1, 0, representation_galaxy[lo + DECODE_LEVEL_NUMBERS_MIN-1]);
			byte hi = game_progress.completed_levels / 10;
			draw_tile(MAP_SIZE_X - 2, 0, representation_galaxy[hi + DECODE_LEVEL_NUMBERS_MIN-1]);
		}
	}

	// draw Adam or shuttle
	if (game_progress.landed_on_world_number == SHUTTLE_IN_SPACE)
		draw_tile(game_progress.galaxy_x, game_progress.galaxy_y, representation_galaxy[DECODE_SHUTTLE]);
	else
		draw_tile(game_progress.galaxy_x, game_progress.galaxy_y, (you_move_direction % 4));

	// finish drawing
	if (!editor_active)
		al_flip_display();
}

extern byte editor_selected_direction;


char *obj_names[] =
{
	"ROBO",
	"SHIP",
	"WALL",
	"BOX ",
	"VOID",
	"KEY ",
	"DOOR",
	"BOLT",
	"BUSH",
	"BAT ",
	"BOMB", 
	"BANG",
	"IMP ",
	"ROCK",
	"ARC",
	"TEXT"
};


char *prop_names[] =
{
	"YOU",
	"WIN",
	"STOP",
	"PUSH",
	"SINK",
	"OPEN",
	"SHUT",
	"PICK",
	"KILL",
	"MOVE",
	"BOOM",
	"TELE",
	"ACID",
	"IRON",
	"MAGNET",
	"WORD",
};

void show_rules()
{
	// draw border over the playfield
	const int rules_width = 400;
	const int pos_x = WINDOW_WIDTH - rules_width - 30;
	al_draw_filled_rounded_rectangle(pos_x, 30,
		WINDOW_WIDTH - 30, 770,
		10, 10,
		al_map_rgb(0, 20, 40));

	int line = 1;
	int i,j;
	for (i = 0; i < TYPE_MAX; ++i)
	{
		for (j = 0; j < PROPERTY_MAX; ++j)
		{
			ObjPropGet(i, j, array_value);
			if (array_value)
			{
				al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 10, line * 30 + 10, 0, obj_names[i]);
				al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 100, line * 30 + 10, 0, "is");
				al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 154, line * 30 + 10, 0, prop_names[j]);
				++line;
			}
		}
	}
	// show has
	for (j = 0; j < TYPE_MAX; ++j)
	{
		for (i = 0; i < TYPE_MAX; ++i)
		{
			if ( obj_has[j] & (one_lshift_lookup[i]) )
			{
				al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 10, line * 30 + 10, 0, obj_names[j]);
					al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 100, line * 30 + 10, 0, "has");
					al_draw_text(font, al_map_rgb(200, 200, 200), pos_x + 174, line * 30 + 10, 0, obj_names[i]);
					++line;
			}
		}
	}
}

bool load_game_progress()
{
	return false;
}

void save_game_progress()
{

}

void game_lost()
{
}

bool drawing_front_buffer;

void game_draw_screen()
{
	int local_type;
	int pos_x;
	int pos_y;
	byte representation;
	byte tile_buffer[MAP_SIZE_Y][MAP_SIZE_X];
	memset(tile_buffer, EMPTY_TILE, sizeof(tile_buffer));

#if (!EDITOR_ENABLED)
	al_clear_to_color(al_map_rgb(20, 20, 40));
#endif
	if (!editor_active)
		show_rules();

	// draw border over the playfield
	al_draw_filled_rounded_rectangle(SCREEN_MAP_POS_X - 10, SCREEN_MAP_POS_Y - 10,
		SCREEN_MAP_POS_X + MAP_SIZE_X * SCREEN_TILE_SIZE + 10, SCREEN_MAP_POS_Y + MAP_SIZE_Y * SCREEN_TILE_SIZE + 10,
		10, 10,
		atari_color_to_allegro_color(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[0]));

	for (int local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;
		local_type = objects.type[local_index];
		pos_x = objects.x[local_index];
		pos_y = objects.y[local_index];
		representation = get_representation(local_type, local_index);

		if (local_type != TYPE_TEXT)
		{
			if (representation_type[local_type] & REPRESENTATION_DIRECTIONS)
				representation += (objects.direction[local_index] & DIR_MASK);

			if (drawing_front_buffer)
			{
				if (representation_type[local_type] & REPRESENTATION_ANIMATED)
				{
					if (representation_type[local_type] & REPRESENTATION_DIRECTIONS)
						representation += 4;
					else
						representation += 1;
				}
			}
		}

		byte previous = tile_buffer[pos_y][pos_x];
		if (previous == EMPTY_TILE ||
			(
				(drawing_front_buffer && representation < previous) ||
				((!drawing_front_buffer) && representation >= previous)				
			)
			)
		{
			draw_tile(pos_x, pos_y, representation);
			tile_buffer[pos_y][pos_x] = representation;
		}


		if (objects.direction[local_index] & DIR_ACTIVE_RULE)
		{
			int dest_x = SCREEN_MAP_POS_X + pos_x * SCREEN_TILE_SIZE;
			int dest_y = SCREEN_MAP_POS_Y + pos_y * SCREEN_TILE_SIZE;

			al_draw_rectangle(dest_x, dest_y, dest_x + SCREEN_TILE_SIZE, dest_y + SCREEN_TILE_SIZE, al_map_rgb(255, 30, 255), 2);
		}


		if (editor_active && editor_selected_direction != DIR_NONE)
			draw_direction(pos_x, pos_y, objects.direction[local_index] & DIR_MASK);
	}

	if (!editor_active)
		al_flip_display();
};

void game_get_action()
{
	bool action_taken = false;
	ALLEGRO_KEYBOARD_STATE keyboard_state;
	you_move_direction = DIR_NONE;

	const int timer_limit = 12;
	while (al_get_timer_count(timer) < timer_limit)
	{
	};

	while (action_taken==false)
	{
		if (al_get_timer_count(game_clock) > 16)
		{
			drawing_front_buffer = !drawing_front_buffer;
			game_draw_screen();
			al_set_timer_count(game_clock, 0);
		}

		al_get_keyboard_state(&keyboard_state);
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_ESCAPE))
		{
			game_phase = LEVEL_QUIT;
			action_taken = true;
		}
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_UP))
		{
			you_move_direction = DIR_UP;
			action_taken = true;
		}
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_DOWN))
		{
			you_move_direction = DIR_DOWN;
			action_taken = true;
		}
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_LEFT))
		{
			you_move_direction = DIR_LEFT;
			action_taken = true;
		}
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_RIGHT))
		{
			you_move_direction = DIR_RIGHT;
			action_taken = true;
		}
		if (al_key_down(&keyboard_state, ALLEGRO_KEY_SPACE) ||
			al_key_down(&keyboard_state, ALLEGRO_KEY_LCTRL) ||
			al_key_down(&keyboard_state, ALLEGRO_KEY_RCTRL) ||
			al_key_down(&keyboard_state, ALLEGRO_KEY_LSHIFT) ||
			al_key_down(&keyboard_state, ALLEGRO_KEY_RSHIFT)
			)
		{
			you_move_direction = DIR_NONE;
			action_taken = true;
		}

	}
	al_set_timer_count(timer, 0);
}


void galaxy_get_action()
{
	bool action_taken = false;
	while (!action_taken) {
		al_wait_for_event(queue, &event);
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
		{
#if (EDITOR_ENABLED)
			game_phase = LEVEL_QUIT;
			action_taken = true;
			break;
#else
			exit(0);
#endif
		}
		else if (event.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			switch (event.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE:
				game_phase = LEVEL_QUIT;
				action_taken = true;
				break;

			case ALLEGRO_KEY_UP:
				if (game_progress.galaxy_y > 0)
				{
					MapGet(game_progress.galaxy_x, game_progress.galaxy_y - 1, local_type);
					if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
						--game_progress.galaxy_y;
				}
				you_move_direction = DIR_UP;
				action_taken = true;
				break;

			case ALLEGRO_KEY_DOWN:
				if (game_progress.galaxy_y < MAP_SIZE_Y - 1)
				{
					MapGet(game_progress.galaxy_x, game_progress.galaxy_y + 1, local_type);
					if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
						++game_progress.galaxy_y;
				}
				you_move_direction = DIR_DOWN;
				action_taken = true;
				break;

			case ALLEGRO_KEY_LEFT:
				if (game_progress.galaxy_x > 0)
				{
					MapGet(game_progress.galaxy_x-1, game_progress.galaxy_y, local_type);
					if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
						--game_progress.galaxy_x;
				}
				action_taken = true;
				you_move_direction = DIR_LEFT;
				break;

			case ALLEGRO_KEY_RIGHT:
				if (game_progress.galaxy_x < MAP_SIZE_X - 1)
				{
					MapGet(game_progress.galaxy_x+1, game_progress.galaxy_y, local_type);
					if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
						++game_progress.galaxy_x;
				}
				you_move_direction = DIR_RIGHT;
				action_taken = true;
				break;

			case ALLEGRO_KEY_SPACE:
			case ALLEGRO_KEY_LSHIFT:
			case ALLEGRO_KEY_RSHIFT:
			case ALLEGRO_KEY_LCTRL:
			case ALLEGRO_KEY_RCTRL:
				action_taken = true;
				if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
					game_phase = GALAXY_TRIGGER;
				break;
			}
		}
	}
	return;
}

void show_error(const char *error)
{
	al_show_native_message_box(
		display,
		dialog_tile,
		"Error:",
		error,
		NULL,
		ALLEGRO_MESSAGEBOX_ERROR
	);
}

void open_atari_tiles_info()
{
#ifdef _MSC_VER
	char buffer[_MAX_PATH] = "";
	char drive[_MAX_DRIVE] = "";
	char dir[_MAX_DIR] = "";
	char fname[_MAX_FNAME] = "";
	char ext[_MAX_EXT] = "";
	_splitpath(level_set_name, drive, dir, fname, ext);
	strncat(buffer, drive, _MAX_PATH);
	if (strlen(drive)!=0)
		strncat(buffer, "\\", _MAX_PATH);
	strncat(buffer, dir, _MAX_PATH);
	if (strlen(dir) != 0)
		strncat(buffer, "\\", _MAX_PATH);
	strncat(buffer, fname, _MAX_PATH);
	strncat(buffer, ".atl", _MAX_PATH);
	if (atari_tiles_info_file_pointer != NULL)
	{
		fclose(atari_tiles_info_file_pointer);
	}
	atari_tiles_info_file_pointer = fopen(buffer, "rb+");
	fread(atari_tiles_info, sizeof(atari_tiles_info), 1, atari_tiles_info_file_pointer);
#else
#error Add additional build platform!
#endif

}

bool open_level_set()
{
	long file_size;
	FILE *fp = fopen(level_set_name, "rb+");
	if (fp == NULL)
	{
		al_show_native_message_box(
			display,
			"Adam Is Me",
			"Error:",
			"Cannot open level set",
			NULL,
			ALLEGRO_MESSAGEBOX_ERROR
		);
		return false;
	}

	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	// TO DO (allow loading set with smaller number of levels? or set is always full and only it's set as parameters?)
	if (file_size != sizeof(struct level_set_header_def) + LEVELS_MAX * sizeof(map))
	{
		char message[2048];
		sprintf(message,
			"Wrong file size of level set! This compilation of editor requires:\n"
			"NUMBER_OF_LEVELS:%d\n"
			"MAP_SIZE_X:%d\n"
			"MAP_SIZE_Y:%d\n",
			LEVELS_MAX, MAP_SIZE_X, MAP_SIZE_Y
		);
		show_error(message);
		return false;
	}
	if (level_set_file_pointer != NULL)
	{
		fclose(level_set_file_pointer);
	}
	level_set_file_pointer = fp;

	open_atari_tiles_info();
	return true;
}

void deinit_platform()
{
	audio_music(MUSIC_DISABLED);
}

void init_platform()
{
	const float FPS = 50;

	al_init();
	al_init_font_addon();
	al_init_ttf_addon();
	al_init_image_addon();
	al_init_primitives_addon();

	al_install_mouse();
	al_install_keyboard();

	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
	display = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);

	al_set_window_title(display, "Adam Is Me - Level Editor");

	// load assets
	font = al_load_ttf_font("font/MapMaker.ttf", 22, 0);

	char filename[50];
	for (int i = 0; i < TILESET_MAX; ++i)
	{
		sprintf(filename, "gfx_atari/%d.png", i);
		ALLEGRO_BITMAP *loaded = al_load_bitmap(filename);
		if (loaded == NULL)
		{
			al_show_native_message_box(
				display,
				dialog_tile,
				"Cannot load TILESET bitmap",
				filename,
				NULL,
				ALLEGRO_MESSAGEBOX_ERROR
			);
			exit(1);
		}
		tiles_atlas[i] = loaded;
		tiles_atlas_indices[i] = al_load_bitmap_flags(filename, ALLEGRO_KEEP_INDEX);

		// convert tiles_atlas to Atari FNT
#if (EDITOR_ENABLED)
		sprintf(filename, "gfx_atari/%d.fnt", i);
		save_tiles_to_font(tiles_atlas_indices[i], filename);

		sprintf(filename, "gfx_atari/%d.inv", i);
		save_inverse_information(tiles_atlas_indices[i], filename);
#endif
	}


	// load palette
	load_atari_palette("gfx_atari/palette/altirra.act");

	// register events

	queue = al_create_event_queue();
	timer = al_create_timer(1.0 / FPS);
	game_clock = al_create_timer(1.0 / FPS);

	al_register_event_source(queue, al_get_mouse_event_source());
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_display_event_source(display));
	al_start_timer(timer);
	al_start_timer(game_clock);

#if (!EDITOR_ENABLED)
	// open file pointer
	level_set_name = "level_set/levels.aim";
	level_set_file_pointer = fopen(level_set_name, "rb+");
#endif
}

void set_palette()
{
	if (atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number > TILESET_MAX)
		atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number = 1;
	remap_palette(tiles_atlas_indices[atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number], tiles_atlas[atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number]);
}

void deinit()
{
	al_destroy_display(display);
	al_destroy_event_queue(queue);

	al_uninstall_keyboard();
	al_uninstall_mouse();
}