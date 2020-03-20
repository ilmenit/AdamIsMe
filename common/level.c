#include "headers.h"
#include "extern.h"

void load_level()
{
	load_level_data();
	decode_level();
	set_palette();
	set_tileset();
}

///////////////////////////////////////////
// level encoding - Galaxy Map
///////////////////////////////////////////
// 8 worlds
// 8 walls
// 8 level numbers
// 8 locks
// 8 backgrounds
// 1 shuttle
///////////////////////////////////////////

void decode_galaxy()
{
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			MapGet(local_x, local_y, local_type);
			if (local_type == DECODE_SHUTTLE)
			{
				if (game_progress.landed_x == 0xFF) // initial loading when game is starting
				{
					game_progress.galaxy_x = local_x;
					game_progress.galaxy_y = local_y;
				}
				if (game_progress.completed_levels != LEVELS_MAX) // EDITOR
					MapSet(local_x, local_y, LEVEL_DECODE_EMPTY);
			}
			else if (local_type >= DECODE_WORLDS_MIN && local_type < DECODE_WORLDS_MAX)
			{
				// local_temp1 is world number
				local_temp1 = local_type - DECODE_WORLDS_MIN;

				// local_temp2 is number of required completed level to unlock
				local_temp2 = 5 * local_temp1;

				if (game_progress.completed_levels < local_temp2)
				{
					// lock this world
					local_temp1 += DECODE_LOCKS_MIN;
					MapSet(local_x, local_y, local_temp1);
				}
			}
		}
	}
}

///////////////////////////////////////////
// level encoding - for levels except Galaxy Map
///////////////////////////////////////////
// 0x00 - 0x0F - objects (wall etc.)
// 0x10 - 0x1F - text - objects
// 0x20 - 0x2F - text - properties
// 0x30 - 0x3x - text - operators (IS, AND)
// 0x3F - empty
// bits 0x40 and 0x80 - 2 bits to encode direction of object (DIR_XXX)
///////////////////////////////////////////

#define DECODING_WORDS 0
#define DECODING_OBJECTS 1

void decode_standard_level()
{
	local_index = 0;
	// set all objects as free to use
	memset(objects.direction, DIR_KILLED, sizeof(objects.direction));

	// local_temp1 = decoding_type
	for (local_temp1 = DECODING_WORDS; local_temp1 <= DECODING_OBJECTS; ++local_temp1)
	{
		local_text_type = LEVEL_DECODE_EMPTY;

		for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
		{
			for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
			{
				MapGet(local_x, local_y, local_type);
				local_temp2 = DECODE_DIRECTION(local_type);
				local_type &= ENCODING_MASK;

				if (local_temp1 == DECODING_WORDS)
				{
					if (local_type < TYPE_MAX || local_type >= (TYPE_MAX + PROPERTY_MAX + TYPE_MAX + OPERATOR_MAX))
					{
						local_type = LEVEL_DECODE_EMPTY;
					}
					else
					{
						local_text_type = local_type;
						local_type = TYPE_TEXT;
						// text - objects
						if (local_text_type >= TYPE_MAX && local_text_type < (TYPE_MAX + TYPE_MAX))
						{
							local_text_type -= TYPE_MAX;
						}
						// text - properties
						else if (local_text_type >= (TYPE_MAX + TYPE_MAX) && local_text_type < (TYPE_MAX + TYPE_MAX + PROPERTY_MAX))
						{
							//local_text_type -= (TYPE_MAX + PROPERTY_MAX);
							//local_text_type |= AS_PROPERTY;
							local_text_type -= TYPE_MAX;
						}
						// text - operators
						else if (local_text_type >= (TYPE_MAX + TYPE_MAX + PROPERTY_MAX) && local_text_type < (TYPE_MAX + TYPE_MAX + PROPERTY_MAX + OPERATOR_MAX))
						{
							local_text_type -= (TYPE_MAX + PROPERTY_MAX + TYPE_MAX);
							local_text_type |= AS_OPERATOR;
						}
					}
				}
				else // decoding objects
				{
					if (local_type >= TYPE_MAX)
						local_type = LEVEL_DECODE_EMPTY;
					else
						local_text_type = local_type;
				}
				// add object to the list
				if (local_type != LEVEL_DECODE_EMPTY)
				{
					objects.type[local_index] = local_type;
					objects.text_type[local_index] = local_text_type;
					objects.x[local_index] = local_x;
					objects.y[local_index] = local_y;
					objects.direction[local_index] = local_temp2;
					++local_index;
#if EDITOR_ENABLED	
					if (local_index >= MAX_OBJECTS)
					{
						last_obj_index = local_index;
						return;
					}
#endif
				}
			}
		}
		if (local_temp1 == DECODING_WORDS)
			last_text_index = local_index;
	}
	last_obj_index = local_index;
	// decode level name
	world_name[0] = 'W' - 64;
	world_name[1] = 'o' - 70;
	world_name[2] = 'r' - 70;
	world_name[3] = 'l' - 70;
	world_name[4] = 'd' - 70;
	world_name[5] = 'N' - 64;
	world_name[6] = 'a' - 70;
	world_name[7] = 'm' - 70;
	world_name[8] = 'e' - 70;
	world_name[9] = 62;
	world_name[10] = '1' + 4;
}

void decode_level()
{
	if (level_number == LEVEL_GALAXY)
		decode_galaxy();
	else
		decode_standard_level();
}
