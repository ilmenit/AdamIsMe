// TODO: reconsider if we should end the game when there is no object with YOU state?
// TODO: Test UNDO as differential (we have free flag 0x80 in DIR_ that we could use as DIR_UNDO which would mean this object got modified and needs to be stored for undo)
//       this way we could lower memory needed for undo but may increase CPU requirement to store DIR_UNDO whenever object data is modified.
//       Often only 1 characteristic (x,y,type...) of object is modified per turn.
// TODO: Separate Atari platform specific things in Editor to prepare it to be multi-platform
// TODO: Make initial build for other CC65 platform (using CONIO)?
// TODO: In rules.c allow to have two texts standing on the same place as like with AND (BOX And BANG Is TELE)
//  How to do it...?
//  IDEA: Mark on helper map MULTIPLE_NOUN or MULTIPLE_PROP and parse this one differently (reading from all objects on this cell)
//        then treat NOUNS or PROP on the same field as connected with "AND"
//        Writing to Map during parsing will influence negatively parsing performance so impact should be checked 
//        This approach however will NOT solve cases of stacking different types of TEXTs (NOUN,PROPS,OPERATORS) on one cell 
//        nor "parallel parsing" where one stacked expressions starts in the middle of other (I assume it's supported in BabaIsYou)
#include "extern.h"
#include "main.h"
#include "platform.h"

#if EDITOR_ENABLED
void init_editor();
void editor_loop();
#endif

#define PLANET_DATA_X 5
#define PLANET_DATA_Y 5

byte planet_data[PLANET_DATA_X * PLANET_DATA_Y] = {
	 DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN ,
	 DECODE_WALLS_MIN, DECODE_LEVEL_NUMBERS_MIN + 0, DECODE_LEVEL_NUMBERS_MIN + 1, DECODE_LEVEL_NUMBERS_MIN + 2, DECODE_WALLS_MIN,
	 DECODE_WALLS_MIN, DECODE_SHUTTLE_LANDED, DECODE_LEVEL_NUMBERS_MIN + 3, DECODE_LEVEL_NUMBERS_MIN + 4, DECODE_WALLS_MIN,
	 DECODE_WALLS_MIN, DECODE_LEVEL_NUMBERS_MIN + 5, DECODE_LEVEL_NUMBERS_MIN + 6, DECODE_LEVEL_NUMBERS_MIN + 7, DECODE_WALLS_MIN,
	 DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN, DECODE_WALLS_MIN
};

void clear_preprocess_helper()
{
	memset(&preproc_helper.min_val, 0xFF, sizeof(preproc_helper.min_val));
	memset(&preproc_helper.max_val, 0x0 , sizeof(preproc_helper.max_val));
}

void galaxy_display_planet()
{
	for (local_y = 0; local_y < PLANET_DATA_Y; ++local_y)
	{
		for (local_x = 0; local_x < PLANET_DATA_X; ++local_x)
		{
			local_type = planet_data[local_y * PLANET_DATA_X + local_x];

			// set specific wall to the planet
			if (local_type >= DECODE_WALLS_MIN && local_type < DECODE_WALLS_MAX)
				local_type += game_progress.landed_on_world_number;

			local_temp1 = game_progress.landed_x + local_x - 1;
			local_temp2 = game_progress.landed_y + local_y - 2;
			MapSet(local_temp1, local_temp2, local_type);
		}
	}

}

void galaxy_takeoff()
{
	load_level();
	audio_music(MUSIC_GALAXY);
	audio_sfx(SFX_TAKE_OFF);
}

// local_temp1 - world number
// local_temp2 - level number
bool is_level_complete()
{
	return game_progress.worlds_state[local_temp1] & (one_lshift_lookup[local_temp2]);
}

void complete_level()
{
	local_temp1 = level_number / LEVELS_PER_WORLD;
	local_temp2 = level_number % LEVELS_PER_WORLD;
	if (is_level_complete()==false)
	{
		game_progress.worlds_state[local_temp1] |= one_lshift_lookup[local_temp2];
		if (game_progress.completed_levels < (WORLDS_MAX * LEVELS_PER_WORLD))
			++game_progress.completed_levels;
	}
} 

void galaxy_check_trigger()
{
	MapGet(game_progress.galaxy_x, game_progress.galaxy_y, local_type);
	if (game_phase == GALAXY_TRIGGER)
	{
		if (local_type == DECODE_SHUTTLE_LANDED)
		{
			game_phase = GALAXY_ONGOING;
			game_progress.landed_on_world_number = SHUTTLE_IN_SPACE;
			galaxy_takeoff();
		}
		else if (local_type >= DECODE_LEVEL_NUMBERS_MIN && local_type < DECODE_LEVEL_NUMBERS_MAX)
		{
			local_temp1 = local_type - DECODE_LEVEL_NUMBERS_MIN;
			level_number = local_temp1 + game_progress.landed_on_world_number * LEVELS_PER_WORLD;
			game_phase = LEVEL_LOAD;
		}
	}
	else if (game_phase == GALAXY_ONGOING)
	{
		if (local_type >= DECODE_WORLDS_MIN && local_type < DECODE_WORLDS_MAX)
		{
			game_progress.landed_on_world_number = local_type - DECODE_WORLDS_MIN;
			game_progress.landed_x = game_progress.galaxy_x;
			game_progress.landed_y = game_progress.galaxy_y;
			audio_sfx(SFX_LANDING);
			galaxy_display_planet();
		}
		else if (local_type == DECODE_EXIT_UNLOCKED)
		{
			game_phase = GAME_COMPLETED;
		}
	}
}

void galaxy_pass()
{
	// during setting of new rules objects are also changed into other objects
	galaxy_draw_screen();
	galaxy_get_action();
	galaxy_check_trigger();
}


void level_pass()
{
	// during setting of new rules objects are also changed into other objects
	game_draw_screen();

	// these are used to override sound effect - often sound effects order depends on order of objects and we have 1 channel
	helpers.something_transformed = false;
	helpers.something_exploded = false;

	set_game_rules(); // on the beginning of level and after move of other objects

	
	// there must be at least one object with YOU state
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		ObjPropGet(objects.type[local_index], PROP_YOU, array_value);
		if (array_value)
			break;
	}
	if (local_index == last_obj_index)
	{
		game_phase = LEVEL_LOST;
		return;
	}
	

	handle_you();

	// after move of the player, so you can break rule related to move of other object (e.g. BAT IS MOVE)
	set_game_rules(); 

	if (rule_exists[PROP_MOVE] || (rule_exists[PROP_MAGNET] && rule_exists[PROP_IRON]))
	{
		handle_move();
		set_game_rules(); // after move of other objects
	}

	handle_interactions();

	if (helpers.something_transformed)
		audio_sfx(SFX_TRANSFORM); 
	else if (helpers.something_exploded)
		audio_sfx(SFX_BOOM); // replace with better sound for changing object type?								
}


void init_new_game()
{
	level_number = 0;
	memset(&game_progress, 0, sizeof(game_progress));
	//game_progress.completed_levels = 0; // here put more for testing purposes
	game_progress.galaxy_x = 0xFF; // position on the galaxy map
	game_progress.galaxy_y = 0xFF; // position on the galaxy map
	game_progress.landed_x = 0xFF; // position on the galaxy map
	game_progress.landed_y = 0xFF; // position on the galaxy map
	game_progress.landed_on_world_number = SHUTTLE_IN_SPACE;
}

void init_level() 
{
	// initial setting of rules, before any move
	game_phase = LEVEL_ONGOING;
	helpers.you_move_at_least_once = false;

	helpers.rules_may_have_changed = true;

	// initial parsing to extract "Something Is WORD", to have it used in the next processing
	set_game_rules(); // on the beginning of level and after move of other objects

	// parse then again
	helpers.rules_may_have_changed = true;

	// set at the beginning if there is something to PICK (to blink WIN objects from start)
	helpers.pick_exists_as_object = false;
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		ObjPropGet(objects.type[local_index], PROP_PICK, array_value);
		if (array_value)
		{
			helpers.pick_exists_as_object = true;
			break;
		}
	}

}

byte current_music = 0xFF;
void level_loop()
{
	byte music_to_play;
	while (game_phase == LEVEL_LOAD || game_phase == LEVEL_ONGOING)
	{
		if (game_phase == LEVEL_LOAD)
		{
			music_to_play = MUSIC_LEVEL_1 + game_progress.landed_on_world_number % MUSIC_LEVEL_MAX;

			// this is to prevent playing old level music during level initialization
			if (music_to_play != current_music)
				audio_music(MUSIC_DISABLED);

			load_level();
			init_level();
			audio_music(music_to_play);
			current_music = music_to_play;
		}
		while (game_phase == LEVEL_ONGOING)
		{
			level_pass();
		}
		if (game_phase == LEVEL_WON)
		{
			audio_sfx(SFX_LEVEL_WON);
			complete_level(); 
		}
		else if (game_phase == LEVEL_LOST)
		{
			if (helpers.you_move_at_least_once)
			{
				audio_sfx(SFX_LEVEL_LOST);
				game_phase = LEVEL_LOAD;
				game_lost();
			}
			else
				game_phase = LEVEL_QUIT;
		}
	}
}

void galaxy_loop()
{
	level_number = LEVEL_GALAXY;
	load_level();

	// if we are in space or game has been loaded
	if (game_progress.landed_on_world_number == SHUTTLE_IN_SPACE || game_phase == GALAXY_ONGOING)
	{
		audio_music(MUSIC_GALAXY);
	}
	// display planet if we are not in space
	if (game_progress.landed_on_world_number != SHUTTLE_IN_SPACE)
	{
		galaxy_display_planet();
		game_phase = GALAXY_ONGOING;
	}

	while (game_phase == GALAXY_ONGOING)
	{
		galaxy_pass();
	}
}


void game_loop()
{
	for(;;)
	{
		galaxy_loop();
		if (game_phase == GAME_COMPLETED)
			break;
		level_loop();
	}
}

void main(void)
{
	init_platform();


#if EDITOR_ENABLED
	init_new_game();
	init_editor();
	editor_loop();
#else
	if (load_game_progress() == false)
	{
		init_new_game();
	}
	game_phase = GALAXY_ONGOING;
	game_loop();
#endif
	deinit_platform();
}