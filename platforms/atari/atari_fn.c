/*
Extended todo:
1. Compile pure code without $4000-$7FFF
2. Add testing read/write to extended ram
3. 

Memory map for use of extended memory:
$2000-$3FFF - GFX segment (screen + dli) - now GFX is $5000-$681D  len:00181E
$4000-$7FFF - Banked CODE, DATA, RODATA in extended memory (no DLI, no screen mem, no music)
$8000-$AFFF - RMT music (segment discarded from loading, loaded as separate sections)
$B000-$BFFF - extended memory handlers and game data + BSS which must be at the end under MEMTOP
->potentially to $D000 when RAM is turned off? (change of RAMTOP required to D0)
*/

#define PLATFORM_ATARI 1
#include <atari.h>
#include <joystick.h>
#include "../../common/extern.h"
#include "../../common/platform.h"
#include "sounds.h"
#include <peekpoke.h>
#include <fcntl.h>
#include <unistd.h>
// this one needed for SFX_VBI_COUNTER
#include "sfx\sfx.h"

#define EMPTY_TILE 126
#define TIMER_VALUE 12
//#define TIMER_VALUE 12
//#define TIMER_VALUE 15

/*****************************************************************************/
/*                               System Data                                 */
/*****************************************************************************/
int file_pointer;

byte joy_status;
byte video_buffer_number = 0;

#define SetChar(x,y,a) *(video_lookup[(y)]+(x))=(a);
#define GetChar(x,y) *(video_lookup[(y)]+(x))

#define SCREEN_SIZE_X 40
#define SCREEN_SIZE_Y 24

/// DOUBLE BUFFERING //////////////////
byte *video_lookup[SCREEN_SIZE_Y];
byte *video_lookup1[SCREEN_SIZE_Y];
byte *video_lookup2[SCREEN_SIZE_Y];

extern byte *video_ptr1;
extern byte *video_ptr2;

extern byte *display_list1;
extern byte *display_list2;

//////// TILESETS

extern byte display_font_page1;
extern byte display_font_page2;
extern byte *game_font_address;
extern byte game_font_page1;
extern byte game_font_page2;
extern byte *galaxy_font_address;
extern byte galaxy_font_page1;
extern byte galaxy_font_page2;

///////// SOUND RELATED
extern bool audio_only_sfx;

/////////

extern void dl_handler(void);

extern byte background_color;

// just one character
#define REPRESENTATION_SINGLE     0x0
// four next characters, according to direction
#define REPRESENTATION_DIRECTIONS 0x1
// animated frame is next tile
#define REPRESENTATION_ANIMATED   0x2

unsigned char *level_file_name = "LEVELS.RIU";
unsigned char *tile_info_file_name = "LEVELS.ATL"; // which tiles and colors are assigned to planets
unsigned char *progress_file_name = "PROGRESS.DAT";
unsigned char *font_file_name = "0.FNT";
unsigned char *inverse_file_name = "0.INV";

struct atari_tiles_info_def atari_tiles_info[WORLDS_MAX + 1];

// we have 64 tiles and encode tiles inverse in 4 bits of each byte in this array
// we could fit 2 tiles in one bytes by related processing code would be bigger than saved 32 bytes.
byte tiles_inverse[TILES_MAX];

byte representation_obj[] = {
	72, // robbo
	70, // ship
	108, // wall
	114, // box
	124, // void
	120, // key
	118, // door
	112, // bolt
	122, // bush
	100, // bat
	110, // bomb
	96, // bang
	104, // imp
	116, // rock
	88, // coil
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
	32, 34, 36, 38, 40, 42, 44, 46,
	48, 50, 52, 54, 56, 58, 60, 62,

	// properties
	0, 2, 4, 6, 8, 10, 12, 14,
	16, 18, 20, 22, 24, 26, 28, 30,

	// operators
	64, 66, 68
};

byte representation_galaxy[] = {
	// worlds
	44, 46, 48, 50 , 52, 54, 56, 58,

	// walls
	108, 110, 112, 114 , 116, 118, 120, 122,

	// level numbers
	14, 16, 18, 20, 22, 24, 26, 28,

	// locks
	76, 78, 80, 82, 84, 86, 88, 90,

	// backgrounds
	64, 66, 68, 70, 72, 74,

	// shuttle
	10, 106
};


bool undo_data_stored = false;
bool undo_data_stored_this_turn = false;
//struct objects_def undo_data;

void wait_for_vblank(void)
{
	asm("lda $14");
wvb:
	asm("cmp $14");
	asm("beq %g", wvb);
}

bool palette_can_be_modified;

byte *system_colors[COLORS_MAX] =
{
	&background_color,
	&OS.color0,
	&OS.color1,
	&OS.color2,
	&OS.color3,
};

void fade_screen_to_black()
{
	palette_can_be_modified = true;
	while (palette_can_be_modified)
	{
		palette_can_be_modified = false;
		for (local_index = 0; local_index < COLORS_MAX; ++local_index)
		{
			if ((*system_colors[local_index] & 0x0F) > 0)
			{
				--(*system_colors[local_index]);
				palette_can_be_modified = true;
			}
			else
				*system_colors[local_index] = 0;
		}
		//wait_time(1); - we wait for VBLANK because our timer can be disabled (it's in RMT playing procedure)
		wait_for_vblank();
	}
}

void fade_palette_to_level_colors()
{
	palette_can_be_modified = false;
	
	local_temp1 = level_number / LEVELS_PER_WORLD;
	for (local_index = 0; local_index < COLORS_MAX; ++local_index)
	{
		if ((*system_colors[local_index] & 0xF0) != (atari_tiles_info[local_temp1].world_colors[local_index] & 0xF0))
		{
			*system_colors[local_index] = atari_tiles_info[local_temp1].world_colors[local_index] & 0xF0;
		}
		if ((*system_colors[local_index]) < atari_tiles_info[local_temp1].world_colors[local_index])
		{
			++(*system_colors[local_index]);
			palette_can_be_modified = true;
		}
	}
	//wait_time(1); - we wait for VBLANK because our timer can be disabled (it's in RMT playing procedure)
	wait_for_vblank();
}


// we are loading the whole world data not to interrupt music
byte world_level_cache[LEVELS_PER_WORLD][MAP_SIZE_Y*MAP_SIZE_X];
byte font_inverse_cache[TILES_MAX];

byte galaxy_level_cache[sizeof(map)];
byte galaxy_inverse_cache[TILES_MAX];

bool galaxy_cached = false;
byte loaded_world_cache = (SHUTTLE_IN_SPACE - 1);

void read_level_from_disk(byte level_id, byte *destination)
{
	off_t offset;
	audio_music(MUSIC_DISABLED);
	offset = sizeof(struct level_set_header_def);
	offset += (off_t)level_id * ((off_t) sizeof(map));
	lseek(file_pointer, offset, SEEK_SET);
	read(file_pointer, destination, sizeof(map));
}

byte loaded_tileset = 0xFF;

void read_font_tileset(byte *font_address, byte *inverse_data)
{
	byte *address;
	audio_music(MUSIC_DISABLED);
	close(file_pointer);

	local_temp1 = level_number / LEVELS_PER_WORLD;
	font_file_name[0] = atari_tiles_info[local_temp1].tileset_number + '0';
	file_pointer = open(font_file_name, O_RDONLY);
	if (file_pointer != -1)
	{
		// read font data to proper addresses
		for (local_index = 0; local_index < 4; ++local_index)
		{
			address = font_address + local_index * (32 * 8);
			read(file_pointer, address, 32 * 8);
			address += 1024;
			read(file_pointer, address, 32 * 8);
		}
		close(file_pointer);
		// read information about inversed characters in tile (+128) 
		inverse_file_name[0] = atari_tiles_info[local_temp1].tileset_number + '0';
		file_pointer = open(inverse_file_name, O_RDONLY);
		if (file_pointer != -1)
		{
			read(file_pointer, inverse_data, TILES_MAX);
			close(file_pointer);
		}
	}
	file_pointer = open(level_file_name, O_RDONLY);
}


void load_level_data()
{
	undo_data_stored = false;
	if (level_number == LEVEL_GALAXY)
	{
		if (!galaxy_cached)
		{
			read_level_from_disk(LEVEL_GALAXY, galaxy_level_cache);
			read_font_tileset(galaxy_font_address, galaxy_inverse_cache);
			galaxy_cached = true;
		}
		memcpy(map, galaxy_level_cache, sizeof(map));
		memcpy(tiles_inverse, galaxy_inverse_cache, sizeof(tiles_inverse));
	}
	else
	{
		if (game_progress.landed_on_world_number != loaded_world_cache)
		{
			local_temp1 = game_progress.landed_on_world_number*LEVELS_PER_WORLD;
			for (local_index = 0; local_index < LEVELS_PER_WORLD; ++local_index)
			{
				read_level_from_disk(local_temp1 + local_index, world_level_cache[local_index]);
			}
			loaded_world_cache = game_progress.landed_on_world_number;
		}
		memcpy(map, world_level_cache[level_number % 8], sizeof(map));

		local_temp1 = level_number / LEVELS_PER_WORLD;
		local_flags = atari_tiles_info[local_temp1].tileset_number;
		if (local_flags !=0 && loaded_tileset != local_flags)
		{
			read_font_tileset(game_font_address, font_inverse_cache);
			loaded_tileset = local_flags;
		}
		memcpy(tiles_inverse, font_inverse_cache, sizeof(tiles_inverse));
	}
	fade_screen_to_black();
}

bool load_game_progress()
{
	audio_music(MUSIC_DISABLED);
	close(file_pointer);

	file_pointer = open(progress_file_name, O_RDONLY);
	if (file_pointer != -1)
	{
		read(file_pointer, &game_progress, sizeof(game_progress));
		close(file_pointer);
		file_pointer = open(level_file_name, O_RDONLY);
		return true;
	}

	file_pointer = open(level_file_name, O_RDONLY);
	return false;
}

void save_game_progress()
{
	audio_music(MUSIC_DISABLED);
	close(file_pointer);

	file_pointer = open(progress_file_name, O_RDWR | O_CREAT);
	write(file_pointer, &game_progress, sizeof(game_progress));
	close(file_pointer);

	file_pointer = open(level_file_name, O_RDONLY);
}

void test_joystick()
{
	joy_status = joy_read(JOY_1);
	if (joy_status == 0)
		return;

	if (JOY_UP(joy_status))
	{
		you_move_direction = DIR_UP;
		return;
	}
	else if (JOY_RIGHT(joy_status))
	{
		you_move_direction = DIR_RIGHT;
		return;
	}
	else if (JOY_DOWN(joy_status))
	{
		you_move_direction = DIR_DOWN;
		return;
	}
	else if (JOY_LEFT(joy_status))
	{
		you_move_direction = DIR_LEFT;
		return;
	}
	else if (JOY_BTN_1(joy_status))
	{
		you_move_direction = DIR_NONE;
		return;
	}
}

void set_timer(byte timer_value)
{
	POKE(SFX_VBI_COUNTER, timer_value);
}

void wait_for_timer()
{
	while (PEEK(SFX_VBI_COUNTER))
	{
		asm("nop");
	}
}

void wait_time(byte t)
{
	set_timer(t);
	wait_for_timer();
}

void wait_for_fire()
{
	do
	{
		wait_time(1);
	} while (!JOY_BTN_1(joy_status));
	do
	{
		wait_time(1);
	} while (JOY_BTN_1(joy_status));
}

void swap_video_buffer()
{
	if (video_buffer_number == 1)
	{
		// Install new display list
		//wait_for_vblank();
		OS.sdlst = &display_list2;
		wait_for_vblank();

		// clear screen
		memset(video_ptr1, EMPTY_TILE, SCREEN_SIZE_X * SCREEN_SIZE_Y);
		// change lookup
		memcpy(video_lookup, video_lookup1, sizeof(video_lookup));
		video_buffer_number = 0;
	}
	else
	{
		// Install new display list
		//wait_for_vblank();
		OS.sdlst = &display_list1;
		wait_for_vblank();

		// clear screen
		memset(video_ptr2, EMPTY_TILE, SCREEN_SIZE_X * SCREEN_SIZE_Y);
		// change lookup
		memcpy(video_lookup, video_lookup2, sizeof(video_lookup));

		video_buffer_number = 1;
	}
}

void set_tileset()
{
	wait_for_vblank();
	// disable ANTIC
	ANTIC.dmactl = 0;
	OS.sdmctl = 0;

	swap_video_buffer();

	if (atari_tiles_info[level_number / LEVELS_PER_WORLD].tileset_number == 0)
	{
		OS.chbas = galaxy_font_page1;
		display_font_page1 = galaxy_font_page1;
		display_font_page2 = galaxy_font_page2;
	}
	else
	{
		OS.chbas = game_font_page1;
		display_font_page1 = game_font_page1;
		display_font_page2 = game_font_page2;
	}
	// enable antic
	OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;  // enable ANTIC
	ANTIC.dmactl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;
}


void set_palette()
{
	palette_can_be_modified = true;
}

byte representation_music[MUSIC_MAX] =
{
	0x22, // MUSIC_DISABLED
	0x0E, // MUSIC_GALAXY
	0x00, // MUSIC_LEVEL_1
	0x12, // MUSIC_LEVEL_2
	0x24, // MUSIC_LEVEL_3
	0x22, // MUSIC_SFX_ONLY
};

byte representation_sfx[SFX_MAX] =
{
	0xFF, // SFX_NONE
	0x1E, // SFX_CLICK
	0x14, // SFX_LEVEL_WON
	0x19, // SFX_LEVEL_LOST
	0x1D, // SFX_LANDING
	0x1C, // SFX_TAKE_OFF
	0x13, // SFX_YOU_MOVE
	0x16, // SFX_SINK
	0x18, // SFX_PICK
	0x1B, // SFX_TELE
	0x15, // SFX_PUSH
	0x1A, // SFX_BOOM
	0x17, // SFX_OPEN
	0x1F, // SFX_TRANSFORM
	0x20, // SFX_SHIP_MOVE
};

void audio_music(unsigned char music_id)
{
	if (music_id == MUSIC_DISABLED)
	{
		stop_music();
	}
	else
	{
		play_music(representation_music[music_id]);
	}
}

void audio_sfx(unsigned char sfx_id)
{
	play_sfx(representation_sfx[sfx_id]);
}

void switch_music()
{
	audio_sfx(SFX_CLICK);
	wait_time(25);
	audio_only_sfx = audio_only_sfx ? false : true;
	if (audio_only_sfx)
	{
		audio_music(MUSIC_SFX_ONLY);
	}
	else
	{
		if (game_progress.landed_on_world_number == SHUTTLE_IN_SPACE)
			audio_music(MUSIC_GALAXY);
		else
			audio_music(MUSIC_LEVEL_1 + game_progress.landed_on_world_number % MUSIC_LEVEL_MAX);
	}
}

byte last_galaxy_move_direction; // this one is displayed

void galaxy_get_action()
{
	bool action_taken = false;
	last_galaxy_move_direction = you_move_direction % 4;

	you_move_direction = DIR_CREATED;

	while (palette_can_be_modified)
		fade_palette_to_level_colors();

	///// TODO: Here implement UNDO if SFX_VBI_COUNTER > 0 or x) ???

	//wait_for_timer();
	while (PEEK(SFX_VBI_COUNTER))
	{
		asm("nop");
		if (OS.rtclok[2] % 16 == 0)
			galaxy_draw_screen();
	}

	while (!action_taken)
	{

		// disable attract mode
		OS.atract = 0;

		if (OS.rtclok[2] % 16 == 0)
			galaxy_draw_screen();

		action_taken = true;
		test_joystick();

		switch (you_move_direction)
		{
		case DIR_UP:
			if (game_progress.galaxy_y > 0)
			{
				local_type = MapGet(game_progress.galaxy_x, game_progress.galaxy_y - 1);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					--game_progress.galaxy_y;
			}
			break;
		case DIR_DOWN:
			if (game_progress.galaxy_y < MAP_SIZE_Y - 1)
			{
				local_type = MapGet(game_progress.galaxy_x, game_progress.galaxy_y + 1);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					++game_progress.galaxy_y;
			}
			break;
		case DIR_LEFT:
			if (game_progress.galaxy_x > 0)
			{
				local_type = MapGet(game_progress.galaxy_x - 1, game_progress.galaxy_y);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					--game_progress.galaxy_x;
			}
			break;
		case DIR_RIGHT:
			if (game_progress.galaxy_x < MAP_SIZE_X - 1)
			{
				local_type = MapGet(game_progress.galaxy_x + 1, game_progress.galaxy_y);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					++game_progress.galaxy_x;
			}
			break;
		case DIR_NONE:
			if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
				game_phase = GALAXY_TRIGGER;
			else
				action_taken = false;
			break;
		case DIR_CREATED:
			action_taken = false;
			last_galaxy_move_direction = DIR_DOWN;
		}

		// keys - escape=28, backspace=52, space=33 - by https://atariwiki.org/wiki/Wiki.jsp?page=Read%20keyboard
		if (OS.ch == 28)
		{
			// exit to main menu?
		}
		switch (GTIA_READ.consol)
		{
		case 0:
			// if SELECT, START and OPTION pressed, then increase completed levels
			if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
				game_progress.completed_levels = 77;
			break;
		case 0x3: // only option
			switch_music();
			break;
		}
	}
	if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
		audio_sfx(SFX_YOU_MOVE);

	last_galaxy_move_direction = you_move_direction % 4;
	set_timer(TIMER_VALUE);
}

void restore_undo_data()
{
	load_level();
	set_palette();
	//memcpy(&objects, &undo_data, sizeof(objects));	
	init_level();	
}

void store_undo_data()
{
	//memcpy(&undo_data, &objects, sizeof(objects));
	undo_data_stored = true;
	undo_data_stored_this_turn = true;
}


void game_lost()
{
	audio_sfx(SFX_LEVEL_LOST);
	//background_color = 0x00;
	OS.color0 = 0x08;
	OS.color1 = 0x0B;
	OS.color2 = 0x04;
	OS.color3 = 0x04;
	game_draw_screen();

	you_move_direction = DIR_CREATED;
	while (you_move_direction != DIR_NONE)
	{
		test_joystick();
	}
	if (undo_data_stored)
		restore_undo_data();
}


void game_get_action()
{
	you_move_direction = DIR_CREATED;

	OS.ch = 0x0; // clear key

	while (palette_can_be_modified)
		fade_palette_to_level_colors();


	wait_for_timer();

	while (you_move_direction == DIR_CREATED && game_phase == LEVEL_ONGOING)
	{

		// disable attract mode
		OS.atract = 0;

		if (OS.rtclok[2] % 16 == 0)
			game_draw_screen();

		test_joystick();

		// keys - escape=28, backspace=52, space=33 - by https://atariwiki.org/wiki/Wiki.jsp?page=Read%20keyboard
		if (OS.ch == 28)
		{
			game_phase = LEVEL_LOST;
			you_move_direction = DIR_NONE;
			return;
		}
		if (GTIA_READ.consol == 3) // OPTION
		{
			switch_music();
		}

		if (you_move_direction == DIR_CREATED) // no directin selected
		{
			if (!undo_data_stored_this_turn)
				store_undo_data();
		}
		else
			undo_data_stored_this_turn = false; // store undo data on the next move
	}
	set_timer(TIMER_VALUE);
	if (you_move_direction != DIR_NONE)
		helpers.you_move_at_least_once = true;
}

#define SCORE_NUTS_ICON 32

// sets tile at map position (local_x, local_y), uses local_temp1 and local_temp2
void draw_tile(byte tile_to_set)
{
	byte tile_inverse = tiles_inverse[(tile_to_set % 128)/2];
	local_temp1 = local_x * 2;
	local_temp2 = local_y * 2;	
	SetChar(local_temp1, local_temp2, tile_to_set + ((tile_inverse & 0x1) ? 128 : 0) );
	SetChar(local_temp1 + 1, local_temp2, tile_to_set + ((tile_inverse & 0x2) ? 129 : 1));
	++local_temp2;
	SetChar(local_temp1, local_temp2, tile_to_set + ((tile_inverse & 0x4) ? 128 : 0));
	SetChar(local_temp1 + 1, local_temp2, tile_to_set + ((tile_inverse & 0x8) ? 129 : 1));
}

void galaxy_draw_screen()
{
	// draw galaxy
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			local_type = MapGet(local_x, local_y);

			// local_temp1 is tile to draw
			if (local_type == LEVEL_DECODE_EMPTY)
			{
				local_temp1 = EMPTY_TILE;
			}
			else if (local_type >= DECODE_LEVEL_NUMBERS_MIN && local_type < DECODE_LEVEL_NUMBERS_MAX)
			{
				local_temp1 = game_progress.landed_on_world_number;
				local_temp2 = local_type - DECODE_LEVEL_NUMBERS_MIN;
				if (is_level_complete())
					local_temp1 = representation_galaxy[local_type] + 128;
				else
					local_temp1 = representation_galaxy[local_type];
			}
			else
			{
				local_temp1 = representation_galaxy[local_type];
				// blink stars
				if (POKEY_READ.random < 16)
					if (local_type >= DECODE_BACKGROUND_MIN + 2 && local_type < DECODE_BACKGROUND_MAX)
						local_temp1 = EMPTY_TILE;
			}

			draw_tile(local_temp1);
		}
	}
	
	if (game_progress.completed_levels > 0)
	{
		// draw number of completed levels
		local_temp1 = (game_progress.completed_levels / 10) * 2; // *2 because 2 chars per tile
		local_x = MAP_SIZE_X - 2;
		local_y = 0;
		draw_tile(local_temp1 + 12 + 128); // 12 is 0
		local_temp1 = (game_progress.completed_levels % 10) * 2;
		++local_x;
		draw_tile(local_temp1 + 12 + 128);
	}
	

	// draw robbo or shuttle
	local_x = game_progress.galaxy_x;
	local_y = game_progress.galaxy_y;

	if (game_progress.landed_on_world_number == SHUTTLE_IN_SPACE)
	{
		local_temp1 = representation_galaxy[DECODE_SHUTTLE];
	}
	else
	{
		local_temp1 = last_galaxy_move_direction * 2;
	}
	if (video_buffer_number == 0)
		local_temp1 += 32;

	draw_tile(local_temp1);

	swap_video_buffer();
}

void game_draw_screen()
{
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		local_type = objects.type[local_index];

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		if (local_type == TYPE_TEXT)
		{
			local_type = objects.text_type[local_index];
			local_temp1 = representation_text[local_type] + 128;
		}
		else // objects
		{
			local_temp1 = representation_obj[local_type];
			local_flags = representation_type[local_type];
			if (local_flags & REPRESENTATION_DIRECTIONS)
				local_temp1 += (objects.direction[local_index] & DIR_MASK) * 2;

			if (video_buffer_number == 0)
			{
				if (local_flags & REPRESENTATION_ANIMATED)
				{
					if (local_flags & REPRESENTATION_DIRECTIONS)
						local_temp1 += 8;
					else
						local_temp1 += 2;
				}
				if (helpers.pick_exists_as_object == false && ObjPropGet(local_type, PROP_WIN))
					local_temp1 += 128;

			}
		}
		if (objects.direction[local_index] & DIR_ACTIVE_RULE)
			local_temp1 += 128;

		// order of drawing is in a way that always lower character is drawn. Last one (126) is EMPTY_TILE
		local_temp2 = GetChar(local_x*2, local_y*2) % 128;
		if (local_temp2 == EMPTY_TILE ||
			(
			((video_buffer_number == 0) && ((local_temp1 % 128) < local_temp2)) ||
				((video_buffer_number == 1) && ((local_temp1 % 128) >= local_temp2))
				)
			)
		{
			draw_tile(local_temp1);
		}
	}

	swap_video_buffer();
}

void print_error(char *s)
{
	byte *video_ptr = OS.savmsc;
	do {
		*(video_ptr++) = *(s++);
	} while (*s != 0xFF);

	OS.ch = 0;
	while (OS.ch == 0);
	exit(1);
}

void open_and_test_file_io()
{
	// read "levels.atl" information about tiles and colors of worlds
	file_pointer = open(tile_info_file_name, O_RDONLY);
	if (file_pointer == -1)
	{
		print_error("cannot" "\x0" "open" "\x0" "levels" "\xe" "atl" "\xFF");
	}
	read(file_pointer, atari_tiles_info, sizeof(atari_tiles_info));
	close(file_pointer);

	file_pointer = open(level_file_name, O_RDONLY);
	if (file_pointer == -1)
	{
		print_error("cannot" "\x0" "open" "\x0" "levels" "\xe" "riu" "\xFF");
	}
	if (lseek(file_pointer, 1, SEEK_SET) != 1)
	{
		print_error("random" "\x0" "file" "\x0" "access" "\x0" "not" "\x0" "supported" "\x0" "on" "\x0" "this" "\x0" "dos" "\x0" "or\x0" "filesystem" "\xFF");
	}
}

void init_platform()
{
	// open file pointer
	open_and_test_file_io();

	// Set keyboard repeat speed
	OS.keyrep = 2;
	OS.krpdel = 8;
	// Turn the keyboard sound
	OS.noclik = 255;

	// set joy
	joy_install(joy_static_stddrv);

	OS.sdmctl = 0;  // disable ANTIC
	wait_for_vblank();

	// init lookups
	for (local_y = 0; local_y < SCREEN_SIZE_Y; ++local_y)
	{
		video_lookup1[local_y] = video_ptr1 + ((uint)local_y) * SCREEN_SIZE_X;
		video_lookup2[local_y] = video_ptr2 + ((uint)local_y) * SCREEN_SIZE_X;
	}
	memset(video_ptr1, EMPTY_TILE, SCREEN_SIZE_X * SCREEN_SIZE_Y);
	swap_video_buffer();

	ANTIC.nmien = NMIEN_VBI;

	//Display list installed, set the interrupt
	wait_for_vblank();

	OS.vdslst = &dl_handler;

	// Enable DLI
	ANTIC.nmien = NMIEN_VBI | NMIEN_DLI;

	// enable antic
	OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;  // enable ANTIC

	// set colors
	OS.color0 = 28;
	OS.color3 = 40; // inverse color0
	OS.color1 = 130;
	OS.color2 = 15;

	OS.color4 = 0;

	// init PMG to cover borders
	GTIA_WRITE.sizem = 0xFF;
	GTIA_WRITE.grafm = 0xFF;
	GTIA_WRITE.hposm0 = 0x20;
	GTIA_WRITE.hposm1 = 0x28;
	GTIA_WRITE.hposm2 = 0xd0;
	GTIA_WRITE.hposm3 = 0xd8;
	GTIA_WRITE.colpm0 = 0x0;
	GTIA_WRITE.colpm1 = 0x0;
	GTIA_WRITE.colpm2 = 0x0;
	GTIA_WRITE.colpm3 = 0x0;
	OS.pcolr0 = 0x0;
	OS.pcolr1 = 0x0;
	OS.pcolr2 = 0x0;
	OS.pcolr3 = 0x0;
	OS.gprior = 0x0;

	// sound
	init_sfx();
}