#include <atari.h>
#include <joystick.h>
#include "../../common/extern.h"
#include "sounds.h"
#include <peekpoke.h>
#include <fcntl.h>
#include <unistd.h>

#define EMPTY_TILE 126
#define TIMER_VALUE 12
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
extern byte game_font_page1;
extern byte game_font_page2;
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
unsigned char *progress_file_name = "PROGRESS.DAT";

byte representation_obj[] = {
	72, // robbo
	70, // ship
	108, // wall
	114 + 128, // box
	124 + 128, // void
	120, // key
	118 + 128, // door
	112, // bolt
	122 + 128, // bush
	100 + 128, // bat 
	110 + 128, // bomb
	96, // bang
	104 + 128, // imp
	116 + 128, // rock
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
	44, 46, 48 + 128, 50 , 52 + 128, 54, 56, 58,

	// walls
	108, 110, 112 + 128, 114 , 116 + 128, 118, 120, 122,

	// level numbers
	14, 16, 18, 20, 22, 24, 26, 28,

	// locks
	76, 78, 80, 82, 84, 86, 88, 90,

	// backgrounds
	64, 66, 68, 70 + 128, 72, 74,

	// shuttle
	10, 106
};

void wait_for_vblank(void)
{
	asm("lda $14");
wvb:
	asm("cmp $14");
	asm("beq %g", wvb);
}

// we are loading the whole world data not to interrupt music
struct level_def world_level_cache[LEVELS_PER_WORLD];
struct level_def galaxy_level_cache;
bool galaxy_cached = false;
byte loaded_world_cache = (SHUTTLE_IN_SPACE - 1);

void read_level_from_disk(byte level_id, struct level_def *destination)
{
	off_t offset = sizeof(struct level_set_header_def);
	offset += (off_t)level_id * ((off_t) sizeof(struct level_def));
	lseek(file_pointer, offset, SEEK_SET);
	read(file_pointer, destination, sizeof(struct level_def));
}

void load_level_data()
{
	if (level_number == LEVEL_GALAXY)
	{
		if (!galaxy_cached)
		{
			audio_music(MUSIC_DISABLED);
			wait_for_vblank();
			read_level_from_disk(LEVEL_GALAXY, &galaxy_level_cache);
			galaxy_cached = true;
		}
		memcpy(&level, &galaxy_level_cache, sizeof(struct level_def));
	}
	else
	{
		if (game_progress.landed_on_world_number != loaded_world_cache)
		{
			audio_music(MUSIC_DISABLED);
			for (local_index = 0; local_index < LEVELS_PER_WORLD; ++local_index)
			{
				read_level_from_disk(game_progress.landed_on_world_number*LEVELS_PER_WORLD + local_index, &world_level_cache[local_index]);
			}
			loaded_world_cache = game_progress.landed_on_world_number;
		}
		memcpy(&level, &world_level_cache[level_number % 8], sizeof(struct level_def));
	}
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


void swap_video_buffer()
{
	if (video_buffer_number == 1)
	{
		// Install new display list
		//wait_for_vblank();
		OS.sdlst = &display_list2;
		wait_for_vblank();

		// clear screen
		memset(video_ptr1, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
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
		memset(video_ptr2, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
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
	//memset(video_ptr1, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
	//memset(video_ptr2, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
	//wait_for_vblank();
	if (level.tileset_number == 0)
	{
		OS.chbas = game_font_page1;
		display_font_page1 = game_font_page1;
		display_font_page2 = game_font_page2;
	}
	else
	{
		OS.chbas = galaxy_font_page1;
		display_font_page1 = galaxy_font_page1;
		display_font_page2 = galaxy_font_page2;
	}
	// enable antic
	OS.sdmctl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;  // enable ANTIC
	ANTIC.dmactl = DMACTL_PLAYFIELD_NORMAL | DMACTL_DMA_FETCH;
}

void set_palette()
{
	wait_for_vblank();
	// set special reg for DLI
	background_color = level.level_colors[0];
	OS.color0 = level.level_colors[1];
	OS.color1 = level.level_colors[2];
	OS.color2 = level.level_colors[3];
	OS.color3 = level.level_colors[4];
	// OS.color4 must be 0
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

void set_timer()
{
	OS.cdtmv3 = TIMER_VALUE;
}

void wait_for_timer()
{
	//if (OS.cdtmv3 != 0)
	//	++background_color;
	//test_joystick();
	while (OS.cdtmv3)
	{
		asm ("nop");
		//test_joystick();
	}
}

void wait_time(byte t)
{
	OS.cdtmv3 = t;
	while (OS.cdtmv3)
	{
		test_joystick();
	}
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
	play_sfx( representation_sfx[sfx_id] );
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

	wait_for_timer();

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
				local_type = MapGet(game_progress.galaxy_x-1, game_progress.galaxy_y);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					--game_progress.galaxy_x;
			}
			break;
		case DIR_RIGHT:
			if (game_progress.galaxy_x < MAP_SIZE_X - 1)
			{
				local_type = MapGet(game_progress.galaxy_x+1, game_progress.galaxy_y);
				if (local_type < DECODE_WALLS_MIN || local_type >= DECODE_WALLS_MAX)
					++game_progress.galaxy_x;
			}
			break;
		case DIR_NONE:
			if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
				game_state = GALAXY_TRIGGER;
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
	set_timer();
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
}

void game_get_action()
{
	you_move_direction = DIR_CREATED;

	OS.ch = 0x0; // clear key

	wait_for_timer();

	while (you_move_direction == DIR_CREATED && game_state == LEVEL_ONGOING)
	{
		// disable attract mode
		OS.atract = 0;

		if (OS.rtclok[2] % 16 == 0)
			game_draw_screen();
		
		test_joystick();

		// keys - escape=28, backspace=52, space=33 - by https://atariwiki.org/wiki/Wiki.jsp?page=Read%20keyboard
		if (OS.ch == 28) 
		{
			game_state = LEVEL_LOST;
			you_move_direction = DIR_NONE;
			return;
		}	
		if (GTIA_READ.consol == 3) // OPTION
		{
			switch_music();
		}
	}
	set_timer();
	if (you_move_direction != DIR_NONE)
		helpers.you_move_at_least_once = true;
}


#define SCORE_NUTS_ICON 32

void galaxy_draw_screen()
{
	// draw galaxy
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			local_type = MapGet(local_x, local_y);
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
					if (local_type >= DECODE_BACKGROUND_MIN+2 && local_type < DECODE_BACKGROUND_MAX)
						local_temp1 = EMPTY_TILE;
			}

			local_temp2 = local_x * 2;
			SetChar(local_temp2, local_y, local_temp1);
			SetChar(local_temp2 + 1, local_y, local_temp1 + 1);
		}
	}
	if (game_progress.completed_levels > 0)
	{
		// draw number of completed levels
		local_temp1 = (game_progress.completed_levels / 10) * 2;
		SetChar(MAP_SIZE_X * 2 - 4, 0, local_temp1 + 12 + 128);
		SetChar(MAP_SIZE_X * 2 - 3, 0, local_temp1 + 13 + 128);
		local_temp1 = (game_progress.completed_levels % 10) * 2;
		SetChar(MAP_SIZE_X * 2 - 2, 0, local_temp1 + 12 + 128);
		SetChar(MAP_SIZE_X * 2 - 1, 0, local_temp1 + 13 + 128);
	}

	// draw robbo or shuttle
	local_x = game_progress.galaxy_x * 2;

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
	SetChar(local_x, game_progress.galaxy_y, local_temp1);
	SetChar(local_x + 1, game_progress.galaxy_y, local_temp1 + 1);

	swap_video_buffer();
}

void game_draw_screen()
{

	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		local_type = obj_type[local_index];

		local_x = obj_x[local_index] * 2;
		local_y = obj_y[local_index];
		if (local_type == TYPE_TEXT)
		{
			local_type = obj_text_type[local_index];
			local_temp1 = representation_text[local_type];
		}
		else // objects
		{
			local_temp1 = representation_obj[local_type];
			local_flags = representation_type[local_type];
			if (local_flags & REPRESENTATION_DIRECTIONS)
				local_temp1 += (obj_direction[local_index] & DIR_MASK) * 2;

			if (video_buffer_number == 0)
			{
				if (local_flags & REPRESENTATION_ANIMATED)
				{
					if (local_flags & REPRESENTATION_DIRECTIONS)
						local_temp1 += 8;
					else
						local_temp1 += 2;
				}
				if (helpers.pick_exists_as_object==false && ObjPropGet(local_type,PROP_WIN))
					local_temp1 += 128;
			}
		}
		// order of drawing is in a way that always lower character is drawn. Last one (126) is EMPTY_TILE
		local_temp2 = GetChar(local_x, local_y) % 128;
		if (local_temp2 == EMPTY_TILE ||
			( 
				( (video_buffer_number == 0) && ((local_temp1 % 128) <  local_temp2) ) ||
				( (video_buffer_number == 1) && ((local_temp1 % 128) >= local_temp2) ) 
			)
		)
		{
			SetChar(local_x, local_y, local_temp1);
			SetChar(local_x + 1, local_y, local_temp1 + 1);
		}

	}

	swap_video_buffer();
}

////////////////////////// Landing

byte planet_land_anim[5][5] = {
	{DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN },
	{DECODE_WALLS_MIN, DECODE_LEVEL_NUMBERS_MIN + 0, DECODE_LEVEL_NUMBERS_MIN + 1, DECODE_LEVEL_NUMBERS_MIN + 2, DECODE_WALLS_MIN},
	{DECODE_WALLS_MIN, DECODE_SHUTTLE_LANDED, DECODE_LEVEL_NUMBERS_MIN + 3, DECODE_LEVEL_NUMBERS_MIN + 4, DECODE_WALLS_MIN},
	{DECODE_WALLS_MIN, DECODE_LEVEL_NUMBERS_MIN + 5, DECODE_LEVEL_NUMBERS_MIN + 6, DECODE_LEVEL_NUMBERS_MIN + 7, DECODE_WALLS_MIN},
	{DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN},
};

void galaxy_land_anim()
{
	for (local_y = 0; local_y < 5; ++local_y)
	{
		for (local_x = 0; local_x < 5; ++local_x)
		{
			local_type = planet_land_anim[local_y][local_x];

			// set specific wall to the planet
			if (local_type >= DECODE_WALLS_MIN && local_type < DECODE_WALLS_MAX)
				local_type += game_progress.landed_on_world_number;
			MapSet(game_progress.landed_x + local_x - 1, game_progress.landed_y + local_y - 2, local_type);
		}
	}
}


////////////////////////// 

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

	for (local_y = 0; local_y < SCREEN_SIZE_Y; ++local_y)
	{
		video_lookup1[local_y] = video_ptr1 + ((uint)local_y) * SCREEN_SIZE_X;
		video_lookup2[local_y] = video_ptr2 + ((uint)local_y) * SCREEN_SIZE_X;
	}
	memset(video_ptr1, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
	memset(video_ptr2, EMPTY_TILE, MAP_SIZE_X*MAP_SIZE_Y * 4);
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
	GTIA_WRITE.colpm0 = 0x15;
	GTIA_WRITE.colpm1 = 0x25;
	GTIA_WRITE.colpm2 = 0x35;
	GTIA_WRITE.colpm3 = 0x45;
	OS.pcolr0 = 0x0;
	OS.pcolr1 = 0x0;
	OS.pcolr2 = 0x0;
	OS.pcolr3 = 0x0;
	OS.gprior = 0x0;

	// sound
	init_sfx();
}
