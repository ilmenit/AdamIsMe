// FIX SAVE AS

#include "../../common/platform.h"
#include "../../common/extern.h"
#include <stdio.h>
#include <string.h>
#include "allegro_fn.h"
#include "atari_gfx.h"
#include "editor.h"

#if EDITOR_ATARI || PLATFORM_ATARI
extern struct atari_tiles_info_def atari_tiles_info[WORLDS_MAX + 1];
extern FILE *atari_tiles_info_file_pointer;
#endif

ALLEGRO_FILECHOOSER *file_chooser;

extern bool editor_active;
extern FILE *level_set_file_pointer;
extern ALLEGRO_EVENT event;
extern ALLEGRO_DISPLAY * display;
extern ALLEGRO_EVENT_QUEUE *queue;
extern ALLEGRO_FONT * font;
ALLEGRO_FONT * font_small;

extern ALLEGRO_BITMAP *tiles[TILES_COLUMNS * TILES_ROWS];

extern byte representation_obj[];
extern byte representation_text[];
extern byte representation_galaxy[];

extern const char *level_set_name;

extern struct rgb atari_palette[128]; // 128 colors

char level_set_name_buffer[_MAX_PATH];
extern const char *dialog_tile;

byte editor_selected_object = 0;
byte editor_selected_color_button = 0;
byte editor_selected_palette_item = 0;
byte editor_selected_tileset_button = 0;
byte editor_selected_direction = DIR_NONE;
byte editor_selected_swap_level = false;

bool redraw_editor = false;
bool redraw_game_screen = false;

int scalled_mouse_x;
int scalled_mouse_y;
int window_width;
int window_height;

byte copied_palette[COLORS_MAX] = { 0xB4, 0x1C, 0x0E, 0x82, 0x00 };

long get_file_size(FILE *fp)
{
	long file_size;
	long current_pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, current_pos, SEEK_SET);
	return file_size;
}

void test_level_click_handler(ui_button *button)
{
	if (level_number == LEVEL_GALAXY)
		return;

	save_level(level_number);

	editor_active = false;
	al_draw_filled_rectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, al_map_rgba(0, 0, 0, 200));

	load_level();
	init_level();
	while (game_phase == LEVEL_ONGOING)
	{
		level_pass();
	}
	select_level();
	editor_active = true;
}

void save_level(byte level_no)
{
	long offset;
	offset = (long) sizeof(struct level_set_header_def);
	offset += (long) level_no * ((long) sizeof(map));
	fseek(level_set_file_pointer, offset, SEEK_SET);
	fwrite(map, sizeof(map), 1, level_set_file_pointer);
	fflush(level_set_file_pointer);

	// save Atari Tiles information
	fseek(atari_tiles_info_file_pointer, 0, SEEK_SET);
	fwrite(atari_tiles_info, sizeof(atari_tiles_info), 1, atari_tiles_info_file_pointer);
}

void create_level_set()
{
	level_set_header.magic[0] = 'I';
	level_set_header.magic[1] = 'l';
	level_set_header.magic[2] = 'm';
	level_set_header.magic[3] = '!'; 
 	level_set_header.number_of_levels = LEVELS_MAX;
	level_set_header.map_size_x = MAP_SIZE_X;
	level_set_header.map_size_y = MAP_SIZE_Y;

	fseek(level_set_file_pointer, 0, SEEK_SET);
	fwrite(&level_set_header, sizeof(level_set_header), 1, level_set_file_pointer);

	for (int i = 0; i < LEVELS_MAX; ++i)
	{
		level_number=i;
		clear_level(true);
		save_level(i);

	}
	fflush(level_set_file_pointer);
	level_number = 0;
	select_level(0);
}


void save_level_set_as_click_handler(ui_button *button)
{
	file_chooser = al_create_native_file_dialog(level_set_name, "Choose a level set name to save (*.riu)", "*.*;*.riu;", ALLEGRO_FILECHOOSER_SAVE);
	al_show_native_file_dialog(display, file_chooser);
	int counter = al_get_native_file_dialog_count(file_chooser);
	if (counter == 0)
	{
		al_destroy_native_file_dialog(file_chooser);
		return;
	}
	const char *new_level_set_name = al_get_native_file_dialog_path(file_chooser, 0);
	if (strlen(new_level_set_name) >= _MAX_PATH)
	{
		show_error("Path too long, extend _MAX_PATH");
		al_destroy_native_file_dialog(file_chooser);
		return;
	}

	FILE *new_fp = fopen(new_level_set_name, "wb+");
	if (new_fp == NULL)
	{
		al_destroy_native_file_dialog(file_chooser);
		show_error("Cannot open file for writing!");
		return;
	}

	// copy level set to new file
	long file_size = get_file_size(level_set_file_pointer);
	byte *buffer = malloc(file_size);

	fseek(level_set_file_pointer, 0, SEEK_SET);
	fread(buffer, file_size, 1, level_set_file_pointer);
	if (fwrite(buffer, file_size, 1, new_fp) != 1)
	{
		fclose(new_fp);
		al_destroy_native_file_dialog(file_chooser);
		show_error("Cannot write to file!");
		return;
	}
	fflush(new_fp);
	free(buffer);
	fclose(level_set_file_pointer);
	level_set_file_pointer = new_fp;
	strncpy(level_set_name_buffer, new_level_set_name, sizeof(level_set_name_buffer));
	al_destroy_native_file_dialog(file_chooser);
}

void import_level_click_handler(ui_button *button)
{
	file_chooser = al_create_native_file_dialog("*.lvl", "Importing single level (*.lvl)", "*.*;*.lvl;", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
	al_show_native_file_dialog(display, file_chooser);
	int counter = al_get_native_file_dialog_count(file_chooser);
	if (counter == 0)
	{
		al_destroy_native_file_dialog(file_chooser);
		return;
	}

	const char *path = al_get_native_file_dialog_path(file_chooser, 0);
	FILE *fp = fopen(path, "rb");
	if (fp == NULL)
	{
		al_destroy_native_file_dialog(file_chooser);
		show_error("Cannot open file for reading!");
		return;
	}
	long file_size = get_file_size(fp);
	if (file_size != sizeof(map))
	{
		al_destroy_native_file_dialog(file_chooser);
		show_error("Wrong size of LVL file!");
		return;
	}
	fread(map, sizeof(map), 1, fp);
	fclose(fp);

	save_level(level_number);
	select_level();
	al_destroy_native_file_dialog(file_chooser);
}

void export_level_click_handler(ui_button *button)
{
	char filename[_MAX_PATH];
	sprintf(filename, "%d%d.lvl", (level_number / LEVELS_PER_WORLD + 1), (level_number % LEVELS_PER_WORLD) + 1);
	file_chooser = al_create_native_file_dialog(filename, "Exporting single level (*.lvl)", "*.*;*.lvl;", ALLEGRO_FILECHOOSER_SAVE);

	al_show_native_file_dialog(display, file_chooser);
	int counter = al_get_native_file_dialog_count(file_chooser);
	if (counter == 0)
	{
		al_destroy_native_file_dialog(file_chooser);
		return;
	}

	const char *path = al_get_native_file_dialog_path(file_chooser, 0);
	FILE *fp = fopen(path, "wb+");
	if (fp == NULL)
	{
		show_error("Cannot open file for writing!");
		al_destroy_native_file_dialog(file_chooser);
		return;
	}
	fwrite(map, sizeof(map), 1, fp);
	fclose(fp);
	al_destroy_native_file_dialog(file_chooser);
}

void new_level_set_click_handler(ui_button *button)
{
	int result = al_show_native_message_box(
		display,
		dialog_tile,
		"Creating new level set?",
		"Are you sure you want to clear all levels?",
		NULL,
		ALLEGRO_MESSAGEBOX_OK_CANCEL);
	if (result == 1)
	{
		clear_level_set();
	}
}

void open_level_set_click_handler(ui_button *button)
{
	file_chooser = al_create_native_file_dialog("*.riu", "Choose a level set to open (*.riu)", "*.*;*.riu;", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
	al_show_native_file_dialog(display, file_chooser);

	int counter = al_get_native_file_dialog_count(file_chooser);
	if (counter == 0)
	{
		al_destroy_native_file_dialog(file_chooser);
		return;
	}

	const char *new_level_set_name = al_get_native_file_dialog_path(file_chooser, 0);
	if (strlen(new_level_set_name) >= _MAX_PATH)
	{
		show_error("Path too long, extend _MAX_PATH");
		al_destroy_native_file_dialog(file_chooser);
		return;
	}
	
	strncpy(level_set_name_buffer, new_level_set_name, sizeof(level_set_name_buffer));
	if (open_level_set()==false)
	{
		al_destroy_native_file_dialog(file_chooser);
		return;
	}
	select_level();
	al_destroy_native_file_dialog(file_chooser);
}

void board_set(int x, int y, byte val)
{
	MapSet(x, y, val);
}

// for cases when only one object is allowed
void board_remove_all(int obj)
{
	for (size_t i = 0; i < _countof(map); ++i)
	{
		if (map[i] == obj)
			map[i] = LEVEL_DECODE_EMPTY;
	}
}

void draw_on_map(int x, int y)
{
	ALLEGRO_MOUSE_STATE state;
	al_get_mouse_state(&state);
	if (state.buttons == 0)
		return;

	if (state.buttons & 2)
	{
		board_set(x, y, LEVEL_DECODE_EMPTY);
	}
	else if (state.buttons & 1)
	{
		// encode tile
		if (level_number == LEVEL_GALAXY)
		{
			// encode
			int encoded_obj;

			if (editor_selected_object == 0) // shuttle
			{
				encoded_obj = DECODE_SHUTTLE;
				board_remove_all(encoded_obj);
			}
			else if (editor_selected_object < 7) // background
			{
				encoded_obj = DECODE_BACKGROUND_MIN + editor_selected_object - 1;
			}
			else if (editor_selected_object < 7+8) // planet
			{
				// place planets only in area where borders can be drawn
				if (x > 1 && x < (MAP_SIZE_X - 3) && y > 1 && y < (MAP_SIZE_Y - 2) )
				{
					encoded_obj = DECODE_WORLDS_MIN + editor_selected_object - 7;
					board_remove_all(encoded_obj);
				}
				else
				{
					return;
				}
			}
			else  // wall
			{
				encoded_obj = DECODE_WALLS_MIN + editor_selected_object - (7 + 8);
			}
			board_set(x, y, encoded_obj);
		}
		else
		{
			// if last object in editor, then it's equal to cleaning
			if (editor_selected_object == TYPE_TEXT) // type_text has no obj representation, so we use it as cleaning
			{
				board_set(x, y, LEVEL_DECODE_EMPTY);
			}
			else
			{
				MapGet(x, y, array_value);
				if (
					(array_value != LEVEL_DECODE_EMPTY) ||
					(last_obj_index < MAX_OBJECTS)
					)
				{
					// encode on standard level
					int encoded_obj = editor_selected_object;
					int dir = DIR_DOWN;
					if (editor_selected_direction != DIR_NONE)
						dir = editor_selected_direction;
					encoded_obj |= ENCODE_DIRECTION(dir);
					board_set(x, y, encoded_obj);
				}
			}
		}
	}
	decode_level();
	redraw_game_screen = true;
}

ui_button editor_menu_buttons[] =
{
	{ BUTTON_MENU_NEW, 0, 0, 60, 30, "New", NO_TILE, true, new_level_set_click_handler},
	{ BUTTON_MENU_OPEN, 70, 0, 75, 30, "Open", NO_TILE, true, open_level_set_click_handler},
	{ BUTTON_MENU_SAVEAS, 155, 0, 125, 30, "Save As", NO_TILE, true, save_level_set_as_click_handler},

	{ BUTTON_MENU_TEST_LEVEL, 950, 180, 210, 30,   "Test level", NO_TILE, true, test_level_click_handler},
	{ BUTTON_MENU_EXPORT_LEVEL, 950, 220, 210, 30, "Export level", NO_TILE, true, export_level_click_handler},
	{ BUTTON_MENU_IMPORT_LEVEL, 950, 260, 210, 30, "Import level", NO_TILE, true, import_level_click_handler},
	{ BUTTON_MENU_CLEAR_LEVEL, 950, 300, 210, 30,  "Clear level", NO_TILE, true, clear_level_click_handler},
	{ BUTTON_MENU_SWAP_LEVEL, 950, 340, 210, 30,   "Swap level", NO_TILE, true, swap_level_click_handler},

	{ BUTTON_MENU_SWAP_LEVEL, 950, 380, 210, 30,   "Build game", NO_TILE, true, build_game_click_handler},
};

ui_button editor_level_buttons[LEVELS_MAX];
ui_button editor_map_object_buttons[TYPE_MAX + TYPE_MAX + PROPERTY_MAX + OPERATOR_MAX];

ui_button editor_map_direction_buttons[5] = {
	{ BUTTON_DIR_DOWN, 560, 700, 34, 34, "", FIRST_DIRECTION_TILE, true, direction_click_handler   },
	{ BUTTON_DIR_LEFT, 520, 660, 34, 34, "", FIRST_DIRECTION_TILE+1, true, direction_click_handler  },
	{ BUTTON_DIR_UP, 560, 620, 34, 34, "", FIRST_DIRECTION_TILE+2, true, direction_click_handler  },
	{ BUTTON_DIR_RIGHT, 600, 660, 34, 34, "", FIRST_DIRECTION_TILE+3, true, direction_click_handler  },
	{ BUTTON_DIR_NONE, 560, 660, 34, 34, "", FIRST_DIRECTION_TILE+4, true, direction_click_handler, true },
};

ui_button editor_map_color_buttons[COLORS_MAX] = {
	{ BUTTON_COLOR0, 30, 620, 32, 32, "0", NO_TILE, true, color_click_handler },
	{ BUTTON_COLOR1, 66, 620, 32, 32, "1", NO_TILE, true, color_click_handler },
	{ BUTTON_COLOR2, 102, 620, 32, 32, "2", NO_TILE, true, color_click_handler },
	{ BUTTON_COLOR3, 138, 620, 32, 32, "3", NO_TILE, true, color_click_handler },
	{ BUTTON_COLOR4, 174, 620, 32, 32, "4", NO_TILE, true, color_click_handler },
};

ui_button editor_palette_items[128]; // palette size

ui_button editor_palette_actions_buttons[] = {
	{ BUTTON_PALETTE_COPY, 30, 658, 240, 32, "Copy palette", NO_TILE, true, copy_palette_click_handler },
	{ BUTTON_PALETTE_PASTE, 30, 696, 240, 32, "Paste palette", NO_TILE, true, paste_palette_click_handler },
};

// we don't allow to set galaxy tileset
ui_button editor_map_tileset_buttons[TILESET_MAX - 1];

ui_button editor_galaxy_object_buttons[GALAXY_OBJECTS_MAX];

ui_button *all_buttons[
	_countof(editor_menu_buttons) +
		_countof(editor_level_buttons) +
		_countof(editor_map_object_buttons) +
		_countof(editor_map_direction_buttons) +
		_countof(editor_map_color_buttons) +
		_countof(editor_map_tileset_buttons) +
		_countof(editor_palette_items) +
		_countof(editor_palette_actions_buttons) +
		_countof(editor_galaxy_object_buttons)
];

// TODO: Atari related - move out of editor.c to atari specific editor extension
void select_palette_item(int new_one)
{
	editor_palette_items[editor_selected_palette_item].selected = false;
	editor_selected_palette_item = new_one;
	editor_palette_items[editor_selected_palette_item].selected = true;
	redraw_editor = true;
}

// TODO: Atari related - move out of editor.c to atari specific editor extension
void select_tileset(int new_one)
{
	editor_map_tileset_buttons[editor_selected_tileset_button].selected = false;
	editor_selected_tileset_button = new_one;
	redraw_editor = true;
	editor_map_tileset_buttons[editor_selected_tileset_button].selected = true;
	atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number = new_one + 1;
	save_level(level_number);
	load_level();
}

// TODO: Atari related - move out of editor.c to atari specific editor extension
void select_color(int new_one)
{
	editor_map_color_buttons[editor_selected_color_button].selected = false;
	editor_selected_color_button = new_one;
	redraw_editor = true;
	if (new_one >= _countof(editor_map_color_buttons))
	{
		// disable palette button to make it less distracting
		for (size_t i = 0; i < _countof(editor_palette_items); ++i)
			editor_palette_items[i].enabled = false;

		// show copy-paste buttons
		for (size_t i = 0; i < _countof(editor_palette_actions_buttons); ++i)
			editor_palette_actions_buttons[i].enabled = true;

		return;
	}
	editor_map_color_buttons[editor_selected_color_button].selected = true;

	int palette_item = atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[editor_selected_color_button] / 2;
	select_palette_item(palette_item);

	// show palette button
	for (size_t i = 0; i < _countof(editor_palette_items); ++i)
		editor_palette_items[i].enabled = true;

	// hide copy-paste buttons
	for (size_t i = 0; i < _countof(editor_palette_actions_buttons); ++i)
		editor_palette_actions_buttons[i].enabled = false;

}

// TODO: Atari related - move out of editor.c to atari specific editor extension
void color_click_handler(ui_button *button)
{
	int new_one = button->button_id - BUTTON_COLOR0;

	// if the same one selected, then deselect
	if (editor_selected_color_button == new_one)
	{
		new_one = COLORS_MAX;
	}
	select_color(new_one);
}

void tileset_click_handler(ui_button *button)
{
	int new_one = button->button_id - BUTTON_TILESET1;
	select_tileset(new_one);
	select_color(COLORS_MAX);
}


void copy_palette_click_handler(ui_button *button)
{
	memcpy(copied_palette, atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors, sizeof(copied_palette));
}

void paste_palette_click_handler(ui_button *button)
{
	memcpy(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors, copied_palette, sizeof(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors));
	set_palette();
	set_tileset();
	redraw_editor = true;
}

void swap_level_click_handler(ui_button *button)
{
	if (editor_selected_swap_level)
	{
		editor_menu_buttons[BUTTON_MENU_SWAP_LEVEL - BUTTON_MENU_FIRST].selected = false;
		editor_selected_swap_level = false;
	}
	else
	{
		editor_menu_buttons[BUTTON_MENU_SWAP_LEVEL - BUTTON_MENU_FIRST].selected = true;
		editor_selected_swap_level = true;
	}
	redraw_editor = true;
}

int file_copy(const char *source, const char *destination)
{
	char    c[4096]; 
	FILE    *stream_R = fopen(source, "rb");
	FILE    *stream_W = fopen(destination, "wb+");   

	while (!feof(stream_R)) {
		size_t bytes = fread(c, 1, sizeof(c), stream_R);
		if (bytes) {
			fwrite(c, 1, bytes, stream_W);
		}
	}
	fclose(stream_R);
	fclose(stream_W);
	return 0;
}

// this one should be written better with error handling
void build_game_click_handler(ui_button *button)
{
	save_level(level_number);
#ifdef _MSC_VER
	char destination[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	// copy level set to build_atr directory
	_splitpath(level_set_name, drive, dir, fname, ext);
	_getcwd(destination, sizeof(destination));
	strncat(destination, "\\build_atr\\", _MAX_PATH);
	strncat(destination, "levels.riu", _MAX_PATH);
	file_copy(level_set_name, destination);

	// copy level set to build_atr directory
	_getcwd(destination, sizeof(destination));
	strncat(destination, "\\build_atr\\", _MAX_PATH);
	strncat(destination, "levels.atl", _MAX_PATH);

	char source[_MAX_PATH] = "";
	strncat(source, drive, _MAX_PATH);
	strncat(source, dir, _MAX_PATH);
	strncat(source, fname, _MAX_PATH);
	strncat(source, ".atl", _MAX_PATH);
	file_copy(source, destination);

	// build game
	_chdir("build_atr");
	system("build-disk.bat");
	_chdir("..");
#else
#error Add additional build platform!
#endif
}

void clear_level_click_handler(ui_button *button)
{
	int result = al_show_native_message_box(
		display,
		dialog_tile,
		"Clearing current level",
		"Are you sure you want to clear this level?",
		NULL,
		ALLEGRO_MESSAGEBOX_OK_CANCEL);
	if (result == 1)
	{
		clear_level(false);
		load_level();
		select_level();
	}
}

void direction_click_handler(ui_button *button)
{
	// deselect previous button
	int new_one = button->button_id - BUTTON_DIR_DOWN;
	editor_map_direction_buttons[editor_selected_direction].selected = false;
	editor_selected_direction = new_one;
	editor_map_direction_buttons[editor_selected_direction].selected = true;

	select_color(COLORS_MAX);
	redraw_editor = true;
}

void palette_click_handler(ui_button *button)
{
	// deselect previous button
	int new_one = button->button_id - BUTTON_PALETTE_FIRST;
	select_palette_item(new_one);
	atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[editor_selected_color_button] = new_one * 2;
	set_palette();
	set_tileset();
	redraw_editor = true;
}

void swap_level(byte old_level, byte new_level)
{
	byte temp_new[MAP_SIZE_Y*MAP_SIZE_X];
	byte temp_old[MAP_SIZE_Y*MAP_SIZE_X];

	long offset_new, offset_old;

	// read new one to temp1
	offset_new = (long) sizeof(struct level_set_header_def);
	offset_new += (long)new_level * ((long) sizeof(map));

	fseek(level_set_file_pointer, offset_new, SEEK_SET);
	fread(temp_new, sizeof(temp_new), 1, level_set_file_pointer);

	// read new one to temp2
	offset_old = (long) sizeof(struct level_set_header_def);
	offset_old += (long)old_level * ((long) sizeof(map));

	fseek(level_set_file_pointer, offset_old, SEEK_SET);
	fread(temp_old, sizeof(temp_old), 1, level_set_file_pointer);

	// write new to old
	fseek(level_set_file_pointer, offset_old, SEEK_SET);
	fwrite(temp_new, sizeof(temp_new), 1, level_set_file_pointer);

	// write old to new
	fseek(level_set_file_pointer, offset_new, SEEK_SET);
	fwrite(temp_old, sizeof(temp_old), 1, level_set_file_pointer);
}

void level_click_handler(ui_button *button)
{
	save_level(level_number);

	editor_level_buttons[level_number].selected = false;

	int new_level_number = (button->button_id - BUTTON_LEVEL_FIRST);
	if (editor_selected_swap_level)
	{
		if (new_level_number != LEVEL_GALAXY && level_number != LEVEL_GALAXY)
			swap_level(level_number, new_level_number);
		editor_menu_buttons[BUTTON_MENU_SWAP_LEVEL - BUTTON_MENU_FIRST].selected = false;
		editor_selected_swap_level = false;

	}
	select_object_to_draw(0);
	level_number = new_level_number;
	select_level();
}

void map_object_click_handler(ui_button *button)
{
	int new_id = button->button_id - BUTTON_MAP_OBJECT_FIRST;
	select_object_to_draw(new_id);
}

void galaxy_object_click_handler(ui_button *button)
{
	int new_id = button->button_id - BUTTON_GALAXY_OBJECT_FIRST;
	select_object_to_draw(new_id);
}

void clear_level(bool clear_palette)
{
	byte default_level_colors[COLORS_MAX] =  { 0xB4, 0x1C, 0x0E, 0x88, 0x00 };
	byte default_galaxy_colors[COLORS_MAX] = { 0x00, 0x84, 0x0E, 0xC6, 0x26 };

	if (clear_palette)
	{
		// set default colors
		if (level_number == LEVEL_GALAXY)
		{
			memcpy(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors, default_galaxy_colors, sizeof(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors));
			atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number = 0;
		}
		else
		{
			memcpy(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors, default_level_colors, sizeof(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors));
			atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number = 1;
		}
	}
	memset(map, LEVEL_DECODE_EMPTY, sizeof(map));
	save_level(level_number);
}

void clear_level_set()
{
	FILE *fp;
	// first open ATL file
	sprintf(level_set_name_buffer, "new-level-pack-%dx%d.atl", MAP_SIZE_X, MAP_SIZE_Y);
	fp = fopen(level_set_name_buffer, "wb+");
	if (fp == NULL)
	{
		char message[2048];
		sprintf(message, "Cannot create new level set file: %s", level_set_name_buffer);
		show_error(message);
		return;
	}
	if (atari_tiles_info_file_pointer != NULL)
		fclose(atari_tiles_info_file_pointer);

	atari_tiles_info_file_pointer = fp;

	// then open actual level set
	sprintf(level_set_name_buffer, "new-level-pack-%dx%d.riu", MAP_SIZE_X, MAP_SIZE_Y);

	fp = fopen(level_set_name_buffer, "wb+");
	if (fp == NULL)
	{
		char message[2048];
		sprintf(message, "Cannot create new level set: %s", level_set_name_buffer);
		show_error(message);
		return;
	}

	if (level_set_file_pointer != NULL)
		fclose(level_set_file_pointer);
	level_set_file_pointer = fp;

	create_level_set();
	redraw_editor = true;
}

void select_level()
{
	load_level();
	editor_level_buttons[level_number].selected = true;
	select_object_to_draw(0);
	select_color(COLORS_MAX);
	select_tileset(atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].tileset_number-1);
	redraw_editor = true;
	redraw_game_screen = true;

	// enable or disable buttons when changing level
	size_t i;
	if (level_number == LEVEL_GALAXY)
	{
		for (i = 0; i < _countof(editor_map_direction_buttons); ++i)
			editor_map_direction_buttons[i].enabled = false;

		for (i = 0; i < _countof(editor_map_object_buttons); ++i)
			editor_map_object_buttons[i].enabled = false;

		for (i = 0; i < _countof(editor_galaxy_object_buttons); ++i)
			editor_galaxy_object_buttons[i].enabled = true;

		for (i = 0; i < _countof(editor_map_tileset_buttons); ++i)
			editor_map_tileset_buttons[i].enabled = false;

		for (i = 0; i < _countof(editor_map_direction_buttons); ++i)
			editor_map_direction_buttons[i].enabled = false;

		editor_menu_buttons[BUTTON_MENU_SWAP_LEVEL - BUTTON_MENU_FIRST].enabled = false;
		editor_menu_buttons[BUTTON_MENU_TEST_LEVEL - BUTTON_MENU_FIRST].enabled = false;
	}
	else
	{
		for (i = 0; i < _countof(editor_map_direction_buttons); ++i)
			editor_map_direction_buttons[i].enabled = true;

		for (i = 0; i < _countof(editor_map_object_buttons); ++i)
			editor_map_object_buttons[i].enabled = true;

		for (i = 0; i < _countof(editor_galaxy_object_buttons); ++i)
			editor_galaxy_object_buttons[i].enabled = false;

		for (i = 0; i < _countof(editor_map_tileset_buttons); ++i)
			editor_map_tileset_buttons[i].enabled = true;

		for (i = 0; i < _countof(editor_map_direction_buttons); ++i)
			editor_map_direction_buttons[i].enabled = true;

		editor_menu_buttons[BUTTON_MENU_SWAP_LEVEL - BUTTON_MENU_FIRST].enabled = true;
		editor_menu_buttons[BUTTON_MENU_TEST_LEVEL - BUTTON_MENU_FIRST].enabled = true;
	}
}

void select_object_to_draw(int new_id)
{
	if (level_number == LEVEL_GALAXY)
	{
		// deselect current
		editor_galaxy_object_buttons[editor_selected_object].selected = false;

		// select new one
		editor_selected_object = new_id;
		editor_galaxy_object_buttons[editor_selected_object].selected = true;
	}
	else
	{
		// we don't have more buttons!
		assert(new_id < TYPE_MAX + TYPE_MAX + PROPERTY_MAX + OPERATOR_MAX);
		assert(editor_selected_object < TYPE_MAX + TYPE_MAX + PROPERTY_MAX + OPERATOR_MAX);

		// deselect current
		editor_map_object_buttons[editor_selected_object].selected = false;

		// select new one
		editor_selected_object = new_id;
		editor_map_object_buttons[editor_selected_object].selected = true;
	}

	select_color(COLORS_MAX);
	redraw_editor = true;
}

void init_editor()
{
	size_t i;
	size_t all = 0;
	ui_button *button;

	font_small = al_load_ttf_font("font/MapMaker.ttf", 18, 0);

	//////// SET-UP GUI BUTTONS

	// add all buttons for quick drawing
	for (i = 0; i < _countof(editor_menu_buttons); ++i)
		all_buttons[all++] = &editor_menu_buttons[i];
	for (i = 0; i < _countof(editor_level_buttons); ++i)
		all_buttons[all++] = &editor_level_buttons[i];
	for (i = 0; i < _countof(editor_map_object_buttons); ++i)
		all_buttons[all++] = &editor_map_object_buttons[i];
	for (i = 0; i < _countof(editor_map_direction_buttons); ++i)
		all_buttons[all++] = &editor_map_direction_buttons[i];
	for (i = 0; i < _countof(editor_map_color_buttons); ++i)
		all_buttons[all++] = &editor_map_color_buttons[i];
	for (i = 0; i < _countof(editor_palette_items); ++i)
		all_buttons[all++] = &editor_palette_items[i];
	for (i = 0 ; i < _countof(editor_map_tileset_buttons) ; ++i)
		all_buttons[all++] = &editor_map_tileset_buttons[i];
	for (i = 0; i < _countof(editor_palette_actions_buttons); ++i)
		all_buttons[all++] = &editor_palette_actions_buttons[i];
	for (i = 0; i < _countof(editor_galaxy_object_buttons); ++i)
		all_buttons[all++] = &editor_galaxy_object_buttons[i];

	assert(all == _countof(all_buttons));

	// set color background
	for (i = 0; i < _countof(all_buttons); ++i)
	{
		all_buttons[i]->color = al_map_rgb(64, 64, 128);
	}

	// set level buttons
	const int level_button_size_x = 42;
	const int level_button_size_y = 30;
	const int levels_in_row = 24;
	int x, y;
	int p, w;

	for (i = 0; i < _countof(editor_level_buttons); ++i)
	{
		button = &editor_level_buttons[i];
		x = i % levels_in_row;
		y = i / levels_in_row;

		button->button_id = BUTTON_LEVEL_FIRST + i;
		button->x = x * (level_button_size_x + 2);
		button->x += 10 * (x / LEVELS_PER_WORLD);
		button->y = y * (level_button_size_y + 4) + 60;
		button->width = level_button_size_x;
		button->height = level_button_size_y;
		button->selected = false;
		button->enabled = true;
		button->click_handler = level_click_handler;
		button->tile_id = NO_TILE;

		p = i % LEVELS_PER_WORLD + 1;
		w = i / LEVELS_PER_WORLD + 1;
		sprintf(button->text, "%d%d", w, p);
	}
	// set galaxy map button
	button = &editor_level_buttons[i - 1];
	sprintf(button->text, "GM");

	// set map objects
	const int texts_in_column = 16; 

	for (i = 0; i < _countof(editor_map_object_buttons); ++i)
	{
		button = &editor_map_object_buttons[i];
		x = i / texts_in_column;
		y = i % texts_in_column;
		button->x = 700 + x * (SCREEN_TILE_SIZE + 18);
		button->y = y * (SCREEN_TILE_SIZE + 5) + 180;
		button->width = SCREEN_TILE_SIZE + 4;
		button->height = SCREEN_TILE_SIZE + 4;
		button->selected = false;
		button->enabled = false;

		if (i < TYPE_MAX)
		{
			if (i == TYPE_TEXT) // last tile change to empty one
				button->tile_id = EMPTY_TILE;
			else
				button->tile_id = representation_obj[i];
		}
		else
			button->tile_id = representation_text[i - TYPE_MAX];

		button->button_id = BUTTON_MAP_OBJECT_FIRST + i;
		button->click_handler = map_object_click_handler;
	}

	// set galaxy objects
	for (i = 0; i < _countof(editor_galaxy_object_buttons); ++i)
	{
		if (i < 7)
		{
			x = 0;
			y = i;
		}
		else
		{
			if (i < 8 + 7)
			{
				x = 1;
				y = i - 7;
			}
			else
			{
				x = 2;
				y = i - (8 + 7);
			}
		}

		button = &editor_galaxy_object_buttons[i];
		button->x = 700 + x * (SCREEN_TILE_SIZE + 18);
		button->y = y * (SCREEN_TILE_SIZE + 5) + 180;
		button->width = SCREEN_TILE_SIZE + 4;
		button->height = SCREEN_TILE_SIZE + 4;
		button->selected = false;
		button->enabled = false;

		if (x == 0)
		{
			if (i == 0)
				button->tile_id = representation_galaxy[DECODE_SHUTTLE];
			else
				button->tile_id = representation_galaxy[y + DECODE_BACKGROUND_MIN - 1];
		}
		else if (x == 1)
			button->tile_id = representation_galaxy[y + DECODE_WORLDS_MIN];
		else if (x == 2)
			button->tile_id = representation_galaxy[y + DECODE_WALLS_MIN];

		button->button_id = BUTTON_GALAXY_OBJECT_FIRST + i;
		button->click_handler = galaxy_object_click_handler;
	}

	// set tileset buttons
	const int tileset_rows = 4;
	const int tileset_button_size = 32;
	for (i = 0; i < _countof(editor_map_tileset_buttons); ++i)
	{
		button = &editor_map_tileset_buttons[i];
		x = i % tileset_rows;
		y = i / tileset_rows;
		button->x = 300 + x * (tileset_button_size + 2);
		button->y = 620 + y * (tileset_button_size + 2);
		button->width = tileset_button_size;
		button->height = tileset_button_size;
		button->selected = false;
		button->button_id = BUTTON_TILESET1 + i;
		sprintf(button->text, "%d", i+1);
		button->enabled = true;
		button->tile_id = NO_TILE;
		button->click_handler = tileset_click_handler;
	}

	// set color selector buttons
	const int color_rows = 8;
	const int palette_item_size = 17;
	for (i = 0; i < _countof(editor_palette_items); ++i)
	{
		button = &editor_palette_items[i];
		x = i / color_rows;
		y = i % color_rows;
		button->x = 30 + x * (palette_item_size + 1);
		button->y = y * (palette_item_size + 1) + 622 + SCREEN_TILE_SIZE;
		button->width = palette_item_size;
		button->height = palette_item_size;
		button->selected = false;
		button->enabled = false;
		button->tile_id = NO_TILE;
		button->color = atari_color_to_allegro_color((byte)i * 2);
		button->click_handler = palette_click_handler;
		button->button_id = BUTTON_PALETTE_FIRST + i;
	}

	///////////// Initialize the rest
	editor_active = true;

	window_width = WINDOW_WIDTH;
	window_height = WINDOW_HEIGHT;


	sprintf(level_set_name_buffer, "new-level-pack-%dx%d.riu", MAP_SIZE_X, MAP_SIZE_Y);
	level_set_name = level_set_name_buffer;

	// if default set does not exists, create it
	FILE *fp = fopen(level_set_name, "rb+");
	if (fp == NULL)
	{
		clear_level_set();
	}

	// try to load default level set
	open_level_set(level_set_name);
	select_level();
	game_progress.completed_levels = LEVELS_MAX;
	game_progress.landed_x = 0; 	// this way we won't replace shuttle TILE in editor with empty tile
}

void draw_button(ui_button *button)
{
	byte r, g, b;

	if (!button->enabled)
		return;

	if (button->button_id >= BUTTON_COLOR0 && button->button_id <= BUTTON_COLOR4)
	{
		byte atari_color = atari_tiles_info[WORLD_OF_LEVEL_NUMBER()].world_colors[button->button_id - BUTTON_COLOR0];
		button->color = atari_color_to_allegro_color(atari_color);
	}

	// draw background
	al_draw_filled_rounded_rectangle(
		button->x, button->y,
		button->x + button->width, button->y + button->height,
		5, 5,
		button->color);

	// draw border
	if (button->selected)
	{
		r = 255;
		g = 0;
		b = 0;
	}
	else
	{
		r = 200;
		g = 200;
		b = 200;
	}

	al_draw_rounded_rectangle(
		button->x, button->y,
		button->x + button->width, button->y + button->height,
		5, 5,
		al_map_rgb(r, g, b), 2);

	if (button->tile_id != NO_TILE)
	{
		int dest_x = button->x + 2;
		int dest_y = button->y + 2;

		if (button->tile_id < _countof(tiles))
			al_draw_bitmap(tiles[button->tile_id], dest_x, dest_y, 0);
	}

	al_draw_text(font, al_map_rgb(255, 255, 255), button->x + 5, button->y + 5, 0, button->text);
}

void draw_rounded_gradient_rect(float x1, float y1, float x2, float y2,
	ALLEGRO_COLOR c1, ALLEGRO_COLOR c2)
{
	ALLEGRO_VERTEX v[] =
	{
		  { x1, y1, 0, 0, 0, c1},
		  { x2, y1, 0, 0, 0, c1},
		  { x1, y2, 0, 0, 0, c2},
		  { x2, y2, 0, 0, 0, c2}
	};
	al_draw_prim(v, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
}

void draw_editor()
{
	int i;

	//al_clear_to_color(al_map_rgb(40, 40, 40));
	draw_rounded_gradient_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, al_map_rgb(40, 40, 64), al_map_rgb(20, 20, 20));

	// draw buttons (from last to first)
	for (i = _countof(all_buttons)-1; i >= 0 ; i--)
		draw_button(all_buttons[i]);

	// draw path
	al_draw_text(font_small, al_map_rgb(255, 255, 255), 320, 0, 0, "Level set:");
	al_draw_text(font_small, al_map_rgb(255, 255, 255), 320, 15, 0, (const char *)level_set_name);

	// draw additional descriptions
	char text[100];

	if (level_number != LEVEL_GALAXY)
	{
		sprintf(text, "World %d, Level %d", (level_number / LEVELS_PER_WORLD) + 1, (level_number % LEVELS_PER_WORLD) + 1);
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 00, 40, 0, text);
		sprintf(text, "World %d Colors", (level_number/LEVELS_PER_WORLD) + 1);
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 30, 600, 0, text);
		sprintf(text, "World %d Tiles", (level_number / LEVELS_PER_WORLD) + 1);
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 300, 600, 0, text);
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 512, 600, 0, "Direction");
		sprintf(text, "Objects %d/%d", last_obj_index, MAX_OBJECTS);
		if (last_obj_index == MAX_OBJECTS)
			al_draw_text(font_small, al_map_rgb(255, 128, 0), 20, 166, 0, text);
		else if (last_obj_index > MAX_OBJECTS - 20)
			al_draw_text(font_small, al_map_rgb(255, 255, 0), 20, 166, 0, text);
		else
			al_draw_text(font_small, al_map_rgb(255, 255, 255), 20, 166, 0, text);
	}
	else
	{
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 00, 40, 0, "Game Map");
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 30, 600, 0, "Map Colors");
	}
	if (editor_palette_items[0].enabled)
	{
		byte color_value = editor_selected_palette_item * 2;
		sprintf(text, "$%02X", color_value);
		al_draw_text(font_small, al_map_rgb(255, 255, 255), 230, 630, 0, text);
	}

	// draw playfied
	al_draw_rounded_rectangle(SCREEN_MAP_POS_X - 10, SCREEN_MAP_POS_Y - 10,
		SCREEN_MAP_POS_X + MAP_SIZE_X * SCREEN_TILE_SIZE + 10, SCREEN_MAP_POS_Y + MAP_SIZE_Y * SCREEN_TILE_SIZE + 10,
		10, 10, al_map_rgb(100, 100, 200), 1);
}

void draw_playfield_grid()
{
	// draw grid
	for (int y = 0; y < MAP_SIZE_Y; ++y)
	{
		for (int x = 0; x < MAP_SIZE_X; ++x)
		{
			al_draw_rectangle(
				SCREEN_MAP_POS_X + x * SCREEN_TILE_SIZE, SCREEN_MAP_POS_Y + y * SCREEN_TILE_SIZE,
				SCREEN_MAP_POS_X + x * SCREEN_TILE_SIZE + SCREEN_TILE_SIZE, SCREEN_MAP_POS_Y + y * SCREEN_TILE_SIZE + SCREEN_TILE_SIZE,
				al_map_rgb(200, 200, 200), 1);
		}
	}
}

bool process_button_handler(ui_button *button)
{
	if (button->enabled == false)
		return false;

	if (button->click_handler == NULL)
		return false;

	if (scalled_mouse_x >= button->x && scalled_mouse_x < (button->x + button->width))
	{
		if (scalled_mouse_y >= button->y && scalled_mouse_y < (button->y + button->height))
		{
			button->click_handler(button);
			return true;
		}
	}
	return false;
}

void process_button_handlers()
{
	size_t i;
	ui_button *button;

	for (i = 0; i < _countof(all_buttons); ++i)
	{
		button = all_buttons[i];
		if (process_button_handler(button))
			return; // only top button processed
	}
}

void draw_if_on_map()
{
	int x, y, tile_x, tile_y;
	if (scalled_mouse_x >= SCREEN_MAP_POS_X && scalled_mouse_x < SCREEN_MAP_POS_X + SCREEN_TILE_SIZE * MAP_SIZE_X &&
		scalled_mouse_y >= SCREEN_MAP_POS_Y && scalled_mouse_y < SCREEN_MAP_POS_Y + SCREEN_TILE_SIZE * MAP_SIZE_Y)
	{
		select_color(COLORS_MAX); // deselect color button and hide palette if we are drawing
		x = scalled_mouse_x - SCREEN_MAP_POS_X;
		y = scalled_mouse_y - SCREEN_MAP_POS_Y;
		tile_x = x / SCREEN_TILE_SIZE;
		tile_y = y / SCREEN_TILE_SIZE;
		draw_on_map(tile_x, tile_y);
	}
}

void editor_loop()
{
	bool action_taken = false;

	redraw_editor = true;
	redraw_game_screen = true;

	bool mouse_pressed = false;

	while (!action_taken)
	{
		if (al_is_event_queue_empty(queue) &&
			(redraw_game_screen || redraw_editor))
		{
			if (redraw_editor)
			{
				draw_editor();
				redraw_editor = false;
				redraw_game_screen = true;
			}
			if (redraw_game_screen)
			{
				// draw game screen
				if (level_number == LEVEL_GALAXY)
					galaxy_draw_screen();
				else
					game_draw_screen();
				draw_playfield_grid();
				redraw_game_screen = false;
			}
			al_flip_display();
		}

		al_wait_for_event(queue, &event);
		ui_button temp;
		byte new_level_number;

		switch (event.type)
		{
		case ALLEGRO_EVENT_KEY_UP:
			switch (event.keyboard.keycode)
			{
			case ALLEGRO_KEY_OPENBRACE:
				if (level_number > 0)
					new_level_number=level_number-1;
				else
					new_level_number = LEVELS_MAX - 1;

				temp.button_id = BUTTON_LEVEL_FIRST + new_level_number;
				level_click_handler(&temp);
				break;
			case ALLEGRO_KEY_CLOSEBRACE:
				if (level_number < LEVELS_MAX - 1)
					new_level_number = level_number + 1;
				else
					new_level_number = 0;

				temp.button_id = BUTTON_LEVEL_FIRST + new_level_number;
				level_click_handler(&temp);
				break;
			}
			break;


		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			int result = al_show_native_message_box(
				display,
				dialog_tile,
				"Exiting editor?",
				"Are you sure you want close the editor?",
				NULL,
				ALLEGRO_MESSAGEBOX_OK_CANCEL);
			if (result == 1)
			{
				save_level(level_number);
				exit(0);
			}
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			// we do not al_acknowledge_resize so the display scales the content (will be useful for large screens)
			window_width = event.display.width;
			window_height = event.display.height;
			redraw_editor = true;
			break;
		case ALLEGRO_EVENT_DISPLAY_EXPOSE:
		case ALLEGRO_EVENT_DISPLAY_FOUND:
		case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
			redraw_editor = true;
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
		case ALLEGRO_EVENT_MOUSE_AXES:
			scalled_mouse_x = event.mouse.x * WINDOW_WIDTH / window_width;
			scalled_mouse_y = event.mouse.y * WINDOW_HEIGHT / window_height;

			if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
			{
				mouse_pressed = true;
				draw_if_on_map();
			}
			else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
			{
				process_button_handlers();
				mouse_pressed = false;
			}
			if (event.type == ALLEGRO_EVENT_MOUSE_AXES)
			{
				if (mouse_pressed)
					draw_if_on_map();
			}
			break;
		}
	}

}