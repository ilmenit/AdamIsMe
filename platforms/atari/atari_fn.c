/*
Memory map for use of extended memory:
$2000-$3FFF - GFX segment (screen + dli) - now GFX is $5000-$681D  len:00181E
-> $3800-$3FF - PMG
$4000-$7FFF - Banked CODE, DATA, RODATA in extended memory (no DLI, no screen mem, no music)
$8000-$AFFF - RMT music (segment discarded from loading, loaded as separate sections)
$B000-$BC20 - extended memory handlers and game data + BSS which must be at the end under MEMTOP
$D800-$E400 - undo data when game is running without extended memory
*/

#define PLATFORM_ATARI 1
#include <atari.h>
#include <joystick.h>
#include "../../common/extern.h"
#include "../../common/platform.h"
#include "sounds.h"
#include <fcntl.h>
#include <unistd.h>
// this one needed for SFX_VBI_COUNTER
#include "sfx\sfx.h"
#include "ram_handler.h"
#include "undo_redo.h"

// PMG is in the GFX segment $3800-$3FFF
#define PM_BASE_PAGE 0x38
/*
#define PM_BASE_ADDR (PM_BASE_PAGE * 0x100)
#define PM_BASE ((unsigned char*) PM_BASE_ADDR)
#define PL_1    ((unsigned char*) (PM_BASE_ADDR+0x400))
#define PL_2    ((unsigned char*) (PM_BASE_ADDR+0x500))
#define PL_3    ((unsigned char*) (PM_BASE_ADDR+0x600))
#define PL_4    ((unsigned char*) (PM_BASE_ADDR+0x700))
*/

#define SHOW_UNDO_ICON() GTIA_WRITE.hposp0 = 0xC5;
#define HIDE_UNDO_ICON() GTIA_WRITE.hposp0 = 0xF0;
#define SHOW_REDO_ICON() GTIA_WRITE.hposp1 = 0xC5;
#define HIDE_REDO_ICON() GTIA_WRITE.hposp1 = 0xF0;

#define TEST_IO 0

#define EMPTY_TILE 126
#define TIMER_VALUE 12

#define INTRO_OPTION       ((unsigned char*)0x00FF)
#define INTRO_OPTION_CONTINUE 0
#define INTRO_OPTION_NEW_GAME 1

/*****************************************************************************/
/*                               System Data                                 */
/*****************************************************************************/
int file_read_pointer = -1;
int file_progress_pointer = -1; // separate file pointer for saving/loading

byte saved_completed_levels = 0;
bool undo_redo_counter=0;

byte joy_status;
byte video_buffer_number = 0;
byte draw_counter = 0;

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
extern byte *text_ptr;

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

/////////

// we start game by showing unlocking of the first world
// which indicates where to land
extern byte unlocking_x;
extern byte unlocking_y;
void show_unlocking_world();

// just one character
#define REPRESENTATION_SINGLE     0x0
// four next characters, according to direction
#define REPRESENTATION_DIRECTIONS 0x1
// animated frame is next tile
#define REPRESENTATION_ANIMATED   0x2

unsigned char *level_file_name = "LEVELS.AIM";
unsigned char *tile_info_file_name = "LEVELS.ATL"; // which tiles and colors are assigned to planets
unsigned char *progress_file_name = "PROGRESS.DAT";
unsigned char *font_file_name = "0.FNT";
unsigned char *inverse_file_name = "0.INV";

struct atari_tiles_info_def atari_tiles_info[WORLDS_MAX + 1];

// we have 64 tiles and encode tiles inverse in 4 bits of each byte in this array
// we could fit 2 tiles in one bytes by related processing code would be bigger than saved 32 bytes.
byte tiles_inverse[TILES_MAX];

byte representation_obj[] = {
	72, // adam
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
	96, 98, 100, 102, 104, 106,

	// shuttle, shuttle landed
	10, 124,
	// exit unlocked
	8,
	// exit lock
	92
};

bool undo_data_stored_this_turn = false;

void set_gray_palette()
{
	OS.color0 = 0x08;
	OS.color1 = 0x0A;
	OS.color2 = 0x04;
	OS.color3 = 0x02;
	// if the background color would make colors invisible
	if ( (background_color & 0xF0) == 0)
		background_color = 0x06;
	set_palette();
	game_draw_screen();
}

/*
void wait_for_vblank_OLD(void)
{
	asm("lda $14");
wvb:
	asm("cmp $14");
	asm("beq %g", wvb);
}
*/

// waiting for VBLANK when NMIEN = 0
// https://atariage.com/forums/topic/141304-wait-for-vblank/?do=findComment&comment=1722440
void wait_for_vblank(void)
{
w1:
	asm("lda $d40b");
	asm("bmi %g", w1);
w2:
	asm("lda $d40b");
	asm("bpl %g", w2);
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

bool pre_disk_io_done = false;

void pre_disk_io()
{
	ANTIC.dmactl = 0;
	OS.sdmctl = 0;
	pause_music();
	wait_for_vblank();
	//audio_music(MUSIC_DISABLED);
	ANTIC.nmien = 0;
	pre_disk_io_done = true;
}

void post_disk_io()
{
	if (pre_disk_io_done==false)
		return;

	pre_disk_io_done = false;
	OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH | DMACTL_DMA_PLAYERS | DMACTL_PMG_SINGLELINE;  // enable ANTIC
	ANTIC.nmien = NMIEN_VBI | NMIEN_DLI;
	continue_music();
}

void fade_to_black_one_step()
{
	HIDE_UNDO_ICON();
	HIDE_REDO_ICON();

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

void fade_screen_to_black()
{
	palette_can_be_modified = true;
	while (palette_can_be_modified)
	{
		fade_to_black_one_step();
	}
}


#define TEXT_OFFSET 19
void fade_palette_to_level_colors()
{
	if (game_phase == LEVEL_ONGOING)
	{
		text_ptr[TEXT_OFFSET] = game_progress.landed_on_world_number + 53;
		text_ptr[TEXT_OFFSET + 1] = 62;
		text_ptr[TEXT_OFFSET + 2] = (level_number % LEVELS_PER_WORLD) + 53;
	}
	else
	{
		text_ptr[TEXT_OFFSET] = 0;
		text_ptr[TEXT_OFFSET + 1] = 0;
		text_ptr[TEXT_OFFSET + 2] = 0;
	}
	local_temp1 = level_number / LEVELS_PER_WORLD;
	while (palette_can_be_modified)
	{
		palette_can_be_modified = false;
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
		if (palette_can_be_modified)
			wait_for_vblank();
	}
	for (local_index = 0; local_index < COLORS_MAX; ++local_index)
		*system_colors[local_index] = atari_tiles_info[local_temp1].world_colors[local_index];
	//wait_time(1); - we wait for VBLANK because our timer can be disabled (it's in RMT playing procedure)
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
	offset = sizeof(struct level_set_header_def);
	offset += (off_t)level_id * ((off_t) sizeof(map));
	lseek(file_read_pointer, offset, SEEK_SET);
	read(file_read_pointer, destination, sizeof(map));
}

byte loaded_tileset = 0xFF;

void read_font_tileset(byte *font_address, byte *inverse_data)
{
	byte *address;
	close(file_read_pointer);

	local_temp1 = level_number / LEVELS_PER_WORLD;
	font_file_name[0] = atari_tiles_info[local_temp1].tileset_number + '0';
	file_read_pointer = open(font_file_name, O_RDONLY);
	if (file_read_pointer != -1)
	{
		// read font data to proper addresses
		for (local_index = 0; local_index < 4; ++local_index)
		{
			address = font_address + local_index * (32 * 8);
			read(file_read_pointer, address, 32 * 8);
			address += 1024;
			read(file_read_pointer, address, 32 * 8);
		}
		close(file_read_pointer);
		// read information about inversed characters in tile (+128) 
		inverse_file_name[0] = atari_tiles_info[local_temp1].tileset_number + '0';
		file_read_pointer = open(inverse_file_name, O_RDONLY);
		if (file_read_pointer != -1)
		{
			read(file_read_pointer, inverse_data, TILES_MAX);
			close(file_read_pointer);
		}
	}
	file_read_pointer = open(level_file_name, O_RDONLY);
}


void load_level_data()
{
	fade_screen_to_black();
	if (level_number == LEVEL_GALAXY)
	{
		if (saved_completed_levels != game_progress.completed_levels)
		{
			save_game_progress();
			saved_completed_levels = game_progress.completed_levels;
		}
		if (galaxy_cached==false)
		{
			pre_disk_io(); 
			read_level_from_disk(LEVEL_GALAXY, galaxy_level_cache);
			read_font_tileset(galaxy_font_address, galaxy_inverse_cache);
			galaxy_cached = true;
		}
		memcpy(map, galaxy_level_cache, sizeof(map));
		memcpy(tiles_inverse, galaxy_inverse_cache, sizeof(tiles_inverse));
	}
	else
	{
		reset_undo();

		if (game_progress.landed_on_world_number != loaded_world_cache)
		{
			local_temp1 = game_progress.landed_on_world_number*LEVELS_PER_WORLD;
			pre_disk_io();
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
			pre_disk_io();
			read_font_tileset(game_font_address, font_inverse_cache);
			loaded_tileset = local_flags;
		}
		memcpy(tiles_inverse, font_inverse_cache, sizeof(tiles_inverse));
	}
	post_disk_io();
}

int read_bytes;
bool load_game_progress()
{
	if ((*INTRO_OPTION) == INTRO_OPTION_NEW_GAME) // INTRO_OPTION_NEW_GAME, INTRO_OPTION_CONTINUE
	  return false;

	pre_disk_io();
	file_progress_pointer = open(progress_file_name, O_RDONLY);
	if (file_progress_pointer != -1)
	{
		read_bytes = read(file_progress_pointer, &game_progress, sizeof(game_progress));
		close(file_progress_pointer);
		post_disk_io();
		if (read_bytes == sizeof(game_progress))
			return true;
		else
			return false;
	}
	// there is no progress file to load
	post_disk_io();
	return false;
}

void save_game_progress()
{
	pre_disk_io();
	file_progress_pointer = open(progress_file_name, O_WRONLY | O_TRUNC);
	write(file_progress_pointer, &game_progress, sizeof(game_progress));

	// seems that for some reason write is buffered and there is no flush procedure to call
	// therefore we need to close and reopen the write file - property of BeWe DOS or general one?
	
	// lseek(file_progress_pointer, 0, SEEK_SET); 

	close(file_progress_pointer);
}

void set_timer(byte timer_value)
{
	MY_POKE(SFX_VBI_COUNTER, timer_value);
}

// music must be playing at this time
void wait_for_timer()
{
	while (MY_PEEK(SFX_VBI_COUNTER))
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
	} while (JOY_BTN_1(joy_status)==false);
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
	OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH | DMACTL_DMA_PLAYERS | DMACTL_PMG_SINGLELINE;  // enable ANTIC
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
byte new_x, new_y;
void galaxy_get_action()
{
	bool action_taken = false;
	last_galaxy_move_direction = you_move_direction % 4;

	you_move_direction = DIR_CREATED;

	if (palette_can_be_modified)
		fade_palette_to_level_colors();

	if (unlocking_x != 0xFF)
		show_unlocking_world();

	// wait for joy fire release - because of e.g. test_quit() not to trigger it again
	do {
		joy_status = joy_read(JOY_1);
		if (OS.rtclok[2] % 16 == 0)
			galaxy_draw_screen();
	} while (JOY_BTN_1(joy_status));

	wait_for_timer();

	while (action_taken==false)
	{

		// disable attract mode
		OS.atract = 0;

		if (OS.rtclok[2] % 16 == 0)
			galaxy_draw_screen();

		action_taken = true;
		joy_status = joy_read(JOY_1);
		if (JOY_UP(joy_status))
		{
			you_move_direction = DIR_UP;
		}
		else if (JOY_RIGHT(joy_status))
		{
			you_move_direction = DIR_RIGHT;
		}
		else if (JOY_DOWN(joy_status))
		{
			you_move_direction = DIR_DOWN;
		}
		else if (JOY_LEFT(joy_status))
		{
			you_move_direction = DIR_LEFT;
		}
		else if (JOY_BTN_1(joy_status))
		{
			you_move_direction = DIR_NONE;
		}

		new_x = game_progress.galaxy_x;
		new_y = game_progress.galaxy_y;
		switch (you_move_direction)
		{
		case DIR_UP:
			if (game_progress.galaxy_y > 0)
				--new_y; 
			break;
		case DIR_DOWN:
			if (game_progress.galaxy_y < MAP_SIZE_Y - 1)
				++new_y;
			break;
		case DIR_LEFT:
			if (game_progress.galaxy_x > 0)
			{
				--new_x;
			}
			break;
		case DIR_RIGHT:
			if (game_progress.galaxy_x < MAP_SIZE_X - 1)
			{
				++new_x;
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
		if (action_taken)
		{
			MapGet(new_x, new_y, local_type);
			if ((local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX) && local_type != DECODE_EXIT_LOCK)
			{
				game_progress.galaxy_x = new_x;
				game_progress.galaxy_y = new_y;
			}
			else
				action_taken = false;
		}

		switch (GTIA_READ.consol)
		{
		case 0:
			// if SELECT, START and OPTION pressed, then increase completed levels
			if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
				game_progress.completed_levels = (WORLDS_MAX* LEVELS_PER_WORLD);
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

bool perform_undo()
{
	set_timer(8);
	HIDE_REDO_ICON();
	if (undo())
	{
		if (undo_redo_counter == 0)
		{
			undo_redo_counter = 1;
			SHOW_UNDO_ICON();
		}
		else
		{
			undo_redo_counter = 0;
			HIDE_UNDO_ICON();
		}

		init_level();
		set_gray_palette();
		audio_sfx(SFX_CLICK);
		wait_for_timer();
		return true;
	}
	else
		HIDE_UNDO_ICON();
	return false;
}

void perform_redo()
{
	set_timer(8);
	HIDE_UNDO_ICON();
	if (redo())
	{
		if (undo_redo_counter == 0)
		{
			undo_redo_counter = 1;
			SHOW_REDO_ICON();
		}
		else
		{
			undo_redo_counter = 0;
			HIDE_REDO_ICON();
		}

		init_level();
		set_gray_palette();
		audio_sfx(SFX_CLICK);
		wait_for_timer();
	}
	else
	{
		HIDE_REDO_ICON();
		audio_sfx(SFX_CLICK);
		you_move_direction = DIR_NONE;
		set_timer(TIMER_VALUE); // normal "waiting move", therefore set standard timer
	}
}

void store_undo_data()
{
	if (undo_data_stored_this_turn==false)
		store_state();
	undo_data_stored_this_turn = true;
}

bool test_quit()
{
	// keys - escape=28, backspace=52, space=33 - by https://atariwiki.org/wiki/Wiki.jsp?page=Read%20keyboard
	byte quit_counter = 10;
	for (;;)
	{
		if (quit_counter == 0 || OS.ch == 28)
		{
			return true;
		}
		joy_status = joy_read(JOY_1);
		if (JOY_BTN_1(joy_status) && JOY_UP(joy_status))
		{
			--quit_counter;
			fade_to_black_one_step();
			wait_for_vblank();
			you_move_direction = DIR_CREATED;
		}
		else
		{
			if (quit_counter < 10)
			{
				palette_can_be_modified = true;
				fade_palette_to_level_colors();
			}
			return false;
		}
	}
}


void game_lost()
{
	bool reload = false;
	audio_sfx(SFX_LEVEL_LOST);
	//background_color = 0x00;
	set_gray_palette();
	for(;;)
	{
		joy_status = joy_read(JOY_1);
		if (JOY_BTN_1(joy_status))
		{
			if (test_quit())
			{
				game_phase = LEVEL_LOAD;
				return;
			}
			if (JOY_LEFT(joy_status))
			{
				if (perform_undo())
					return;
				else
					reload = true;
			}

		}
		if (OS.ch == 28)
			reload = true;
		if (reload)
		{
			load_level();
			init_level();
			return;
		}
	}
}


// we don't have two button gamepad on Atari, therefore as buttons we use FIRE + JOY_LEFT or JOY_RIGHT
void game_get_action()
{
	you_move_direction = DIR_CREATED;

	OS.ch = 0x0; // clear key

	GTIA_WRITE.hposp0 = 0xF0; // hide undo icon
	GTIA_WRITE.hposp1 = 0xF0; // hide redo icon

	if (palette_can_be_modified)
		fade_palette_to_level_colors();

#if STORE_UNDO_OPTIMIZATION
	if (MY_PEEK(SFX_VBI_COUNTER) > 0 || helpers.you_move_at_least_once == false)
	{
		store_undo_data(); // store only when not slowing down
		wait_for_timer();
	}
#else
	store_undo_data(); // store always
	wait_for_timer();
#endif

	while (you_move_direction == DIR_CREATED && game_phase == LEVEL_ONGOING)
	{
		// disable attract mode
		OS.atract = 0;

		if (OS.rtclok[2] % 16 == 0)
			game_draw_screen();

		joy_status = joy_read(JOY_1);
		if (JOY_UP(joy_status))
		{
			you_move_direction = DIR_UP;
		}
		else if (JOY_RIGHT(joy_status))
		{
			if (JOY_BTN_1(joy_status))
				perform_redo();
			else
				you_move_direction = DIR_RIGHT;
		}
		else if (JOY_DOWN(joy_status))
		{
			you_move_direction = DIR_DOWN;
		}
		else if (JOY_LEFT(joy_status))
		{
			if (JOY_BTN_1(joy_status))
				perform_undo();
			else
				you_move_direction = DIR_LEFT;
		}
		if (test_quit())
		{
			if (helpers.you_move_at_least_once)
				game_phase = LEVEL_LOAD;
			else
				game_phase = LEVEL_QUIT;
			//you_move_direction = DIR_NONE;
			return;
		}
		if (GTIA_READ.consol == 3) // OPTION
		{
			switch_music();
		}

		if (you_move_direction == DIR_CREATED) // no directin selected
		{
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
	local_temp1 = local_y * 2;
	array_ptr = video_lookup[local_temp1];

	local_temp1 = tile_to_set;
	lookup_index = tiles_inverse[(local_temp1 % 128)/2];
	array_ptr += local_x * 2;

	*array_ptr = local_temp1 + ((lookup_index & 0x1) ? 128 : 0);
	array_ptr[1] = local_temp1 + ((lookup_index & 0x2) ? 129 : 1);
	array_ptr[SCREEN_SIZE_X] = local_temp1 + ((lookup_index & 0x4) ? 128 : 0);
	array_ptr[SCREEN_SIZE_X+1] = local_temp1 + ((lookup_index & 0x8) ? 129 : 1);
}

void galaxy_draw_screen()
{
	++draw_counter;
	// draw galaxy
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			MapGet(local_x, local_y, local_type);

			// local_temp1 is tile to draw
			if (local_type == LEVEL_DECODE_EMPTY)
			{
				local_temp1 = EMPTY_TILE;
			}
			else if (local_type == DECODE_EXIT_UNLOCKED)
			{
				local_temp1 = representation_galaxy[local_type];
				if ( (draw_counter%2) == 0)
					local_temp1 += 32;
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
					if (local_type >= DECODE_BACKGROUND_MIN + 2 && local_type < DECODE_BACKGROUND_MIN+4)
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
	

	// draw adam or shuttle
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
	if ((draw_counter%2)==0)
		local_temp1 += 32;

	draw_tile(local_temp1);

	swap_video_buffer();
}


void show_unlocking_world()
{
	static byte previously_completed_levels = 0xFF;
	byte unlocking_tile;
	byte unlock_timer;
	byte unlock_index;
	byte unlock_sub;

	if (previously_completed_levels == game_progress.completed_levels)
		return;

	previously_completed_levels = game_progress.completed_levels;

	unlock_index = 0;
	unlock_sub = 5;
	MapGet(unlocking_x, unlocking_y, unlocking_tile);
	for (unlock_timer = 30; unlock_timer != 0; unlock_timer-= unlock_sub)
	{
		wait_time(unlock_timer);
		if (unlock_index%2 == 0)
			unlocking_tile += DECODE_LOCKS_MIN;
		else
		{
			unlocking_tile -= DECODE_LOCKS_MIN;
			--unlock_sub;
			if (unlock_sub == 0)
				unlock_sub = 1;
		}
		MapSet(unlocking_x, unlocking_y, unlocking_tile);
		galaxy_draw_screen();
		++unlock_index;
	}

	// finish
	unlocking_x = 0xFF;
}


void game_draw_screen()
{
	++draw_counter;
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

			if ((draw_counter % 2) == 0)
			{
				if (local_flags & REPRESENTATION_ANIMATED)
				{
					if (local_flags & REPRESENTATION_DIRECTIONS)
						local_temp1 += 8;
					else
						local_temp1 += 2;
				}
				ObjPropGet(local_type, PROP_WIN, array_value);
				if (helpers.pick_exists_as_object == false && array_value)
					local_temp1 += 128;

			}
		}
		if (objects.direction[local_index] & DIR_ACTIVE_RULE)
			local_temp1 += 128;

		// order of drawing is in a way that always lower character is drawn. Last one (126) is EMPTY_TILE
		local_temp2 = GetChar(local_x*2, local_y*2) % 128;
		if (local_temp2 == EMPTY_TILE ||
			(
			(((draw_counter % 2) == 0) && ((local_temp1 % 128) < local_temp2)) ||
				(((draw_counter % 2) == 1) && ((local_temp1 % 128) >= local_temp2))
				)
			)
		{
			draw_tile(local_temp1);
		}
	}

	swap_video_buffer();
}

#if TEST_IO
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
#endif

void open_and_test_file_io()
{
	// read "levels.atl" information about tiles and colors of worlds
	file_read_pointer = open(tile_info_file_name, O_RDONLY);
#if TEST_IO
	if (file_pointer == -1)
	{
		print_error("cannot" "\x0" "open" "\x0" "levels" "\xe" "atl" "\xFF");
	}
#endif
	read(file_read_pointer, atari_tiles_info, sizeof(atari_tiles_info));
	close(file_read_pointer);

	file_read_pointer = open(level_file_name, O_RDONLY);
#if TEST_IO
	if (file_pointer == -1)
	{
		print_error("cannot" "\x0" "open" "\x0" "levels" "\xe" "aim" "\xFF");
	}
	if (lseek(file_pointer, 1, SEEK_SET) != 1)
	{
		print_error("random" "\x0" "file" "\x0" "access" "\x0" "not" "\x0" "supported" "\x0" "on" "\x0" "this" "\x0" "dos" "\x0" "or\x0" "filesystem" "\xFF");
	}
#endif
}

void rom_copy();

void deinit_platform()
{
	audio_music(MUSIC_SFX_ONLY);
	audio_sfx(SFX_LEVEL_WON);
	wait_time(50);
	fade_screen_to_black();
	wait_time(50);
	audio_music(MUSIC_DISABLED);
	pre_disk_io();
	deinit_sfx();
}

void init_platform()
{
	// sound
	init_sfx();
	pre_disk_io(); // disable Antic and NMIEN

	// read open file pointer
	open_and_test_file_io();
	memory_handler_init();

	// Set keyboard repeat speed
	OS.keyrep = 2;
	OS.krpdel = 8;
	// Turn the keyboard sound
	OS.noclik = 255;

	// set joy
	joy_install(joy_static_stddrv);

	// init lookups
	for (local_y = 0; local_y < SCREEN_SIZE_Y; ++local_y)
	{
		video_lookup1[local_y] = video_ptr1 + ((uint)local_y) * SCREEN_SIZE_X;
		video_lookup2[local_y] = video_ptr2 + ((uint)local_y) * SCREEN_SIZE_X;
	}
	//memset(video_ptr1, EMPTY_TILE, SCREEN_SIZE_X * SCREEN_SIZE_Y);
	swap_video_buffer();

	rom_copy();

	OS.vdslst = &dl_handler;

	// set colors
	OS.color0 = 0;
	OS.color3 = 0; // inverse color0
	OS.color1 = 0;
	OS.color2 = 0;
	OS.color4 = 0;

	// init PMG to cover borders
	GTIA_WRITE.sizem = 0xFF;
	GTIA_WRITE.grafm = 0xFF; // shape of Missiles

	// cover position
	GTIA_WRITE.hposm2 = 0x22; // left cover
	GTIA_WRITE.hposp2 = 0x2A; // left corners
	GTIA_WRITE.hposp3 = 0xCE; // right corners
	GTIA_WRITE.hposm3 = 0xD6; // right cover

	// black corners and black cover of colbak on the sides
	OS.pcolr2 = 0x00;
	OS.pcolr3 = 0x00;

	// other sprites - rewind and forward icons, place out of screen for now
	OS.pcolr0 = 0xB8;
	OS.pcolr1 = 0xB8;
	GTIA_WRITE.hposp0 = 0xf0; // d2
	GTIA_WRITE.hposp1 = 0xf0; // d2

	// missiles are out of screen
	GTIA_WRITE.hposm0 = 0xF0;
	GTIA_WRITE.hposm1 = 0xF0;

	GTIA_WRITE.gractl = GRACTL_PLAYERS; // we use memory as Players shape, not grafp
		
	ANTIC.pmbase = PM_BASE_PAGE; // 0x38

	OS.gprior = 0x1; // 4
}