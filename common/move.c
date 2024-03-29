/*
 Oh boy, making this one was HARD to make it both efficient and working as expected :)
 - The code is hardly readable due to heavy optimizations (it's the most time critical part in the game engine)
 - Open/Shut has higher priority than STOP
 - Difference from BabaIsYou - Open/Shut is evaluated after move, therefore pushed objects need to wait to enter cleaned space
 - Difference from BabaIsYou - Push has higher priority than Open/Shut, therefore until object can be pushed, it won't open/close
 - Magnets makes IRON objects moving, and moving is used in the apply_force, so magnets have magnet_preprocess. 
   They work through STOP (not like in Atari Robbo but like in real-life), because STOP property is done in other preprocess...
*/
#include "extern.h"

#define PREPROCESS_NONE		0x0

// Preprocessing move
#define PREPROCESS_MOVING	0x1
#define PREPROCESS_PUSH		0x2
#define PREPROCESS_STOP		0x4
#define PREPROCESS_FINISH   0x8

// additional flags
#define PREPROCESS_OPEN     0x10
#define PREPROCESS_SHUT     0x20
#define PREPROCESS_MOVE_ACCEPTED 0x40
#define PREPROCESS_MAGNET   0x80

byte move_direction;
byte preprocess_type;

// MOVE
/*
	// Standard move and push

	initial(objects)
	.=...####@..=.
	.====####@..=.

	// preprocess - set on map helping markers (can move for PUSH, moving for MOVE, STOP for wall)
	.S...PPPPM..S.
	.SSSSPPPPM..S.

	// process phase 1 - apply force - from right to left
	// change all PUSH to MOVE if one to the right is MOVE
	// if NONE and to the right is MOVE, then make it FINISH
	.S..FMMMMM..S.
	.SSSSMMMMM..S.

	// process phase 2 - pass FINISH info back
	// from left to right mark all MOVE as OK
	.S..FFFFFF..S.
	.SSSSMMMMM..S.

	// process phase 3 - changing position
	// change position of objects that stay on FINISH

	////////////////////////////////////////////////////////
	// Move and push with Open (Key) /shUt (Door) where Door is also STOP
	////////////////////////////////////////////////////////
	.=...DK##@..=.

	// preprocess - set on map helping markers 
	.S...SPPPM..S. - values
	.....UO....... - flags

	// process phase 1 - apply force - from right to left
	// STANDARD: if to the right is MOVE and if current is PUSH then make it MOVE 
	// STANDARD: if to the right is MOVE and if current is NONE then make it FINISH
	// OPENSHUT: if to the right is MOVE and (if has flags SHUT and to he right has flags OPEN or OPEN and to the right has SHUT), then make it FINISH
	.S...FMMMM..S.
	.....UO....... - flags

	*/

void check_force()
{
	// local_temp1 is the previous one 
	// local_type is the current one
	local_type = map[map_index];

	// if previous (to the right in "right to left") is moving
	if ( local_temp1 & PREPROCESS_MOVING)
	{
		// if PUSH then it's moving, however if it's set as FINISH by OPEN/SHUT, then it's not moving
		if ( local_type & PREPROCESS_PUSH && ((local_type & PREPROCESS_FINISH)==0) )
		{
			local_type |= PREPROCESS_MOVING;
		}
		else if ( ! (local_type & PREPROCESS_STOP) )
		{
			local_type |= PREPROCESS_FINISH;
		}
	}
	else
	{
		// if previous one is not moving, then this one is losing PUSH flag
		local_type &= (~PREPROCESS_PUSH);
	}

	map[map_index] = local_type;
	local_temp1 = local_type;
}

void pass_info_back()
{
	local_type = map[map_index];

	// storing these flags temporary to pass to the next check_force as previous flags 
	local_flags = local_type & (PREPROCESS_OPEN | PREPROCESS_SHUT);

	if (local_type & PREPROCESS_OPEN)
	{
		if ((local_temp1 & PREPROCESS_SHUT)==0)
			local_type &= (~PREPROCESS_OPEN);
	}
	if (local_type & PREPROCESS_SHUT)
	{
		if ((local_temp1 & PREPROCESS_OPEN)==0)
			local_type &= (~PREPROCESS_SHUT);
	}

	// local_temp1 is previous
	// local_type is current
	if ( (local_temp1 & PREPROCESS_FINISH) || (local_temp1 & PREPROCESS_MOVE_ACCEPTED) )
	{		
		if ( local_type & PREPROCESS_MOVING)
		{
			local_type |= PREPROCESS_MOVE_ACCEPTED;
		}
	}
	map[map_index] = local_type;
	local_temp1 = local_type | local_flags;
}

void apply_force()
{
	switch (move_direction)
	{
	case DIR_LEFT:
		for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
		{
			// check if there is nothing in this line that moves
			if (preproc_helper.min_val.x[local_y] == 0xFF)
				continue;

			// start from the right-most object
			local_x = preproc_helper.max_val.x[local_y];
			map_index = MapGetIndex(local_x, local_y);
			local_temp1 = map[map_index];

			// first one is for sure not pushed, so we remove this flag from map
			array_value = local_temp1 & (~PREPROCESS_PUSH);
			map[map_index] = array_value;
			--local_x;

			for (; local_x < MAP_SIZE_X; --local_x)
			{
				--map_index;
				check_force();
			}

			map_index = MapGetIndex(0, local_y);
			local_temp1 = map[map_index];
			array_value = local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN));
			map[map_index] = array_value;

			for (local_x = 1; local_x < MAP_SIZE_X; ++local_x)
			{
				++map_index;
				pass_info_back();
			}
		}
		break;
	case DIR_RIGHT:
		for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
		{
			local_x = preproc_helper.min_val.x[local_y];
			if (local_x == 0xFF)
				continue;

			map_index = MapGetIndex(local_x, local_y);
			local_temp1 = map[map_index];
			array_value = local_temp1 & (~PREPROCESS_PUSH);
			map[map_index] = array_value;

			++local_x;
			for (; local_x < MAP_SIZE_X; ++local_x)
			{
				++map_index;
				check_force();
			}
			map_index = MapGetIndex(MAP_SIZE_X - 1, local_y);
			local_temp1 = map[map_index];
			array_value = local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN));
			map[map_index] = array_value;

			for (local_x = MAP_SIZE_X - 2; local_x < MAP_SIZE_X; --local_x)
			{
				--map_index;
				pass_info_back();
			}
		}
		break;
	case DIR_DOWN:
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			local_y = preproc_helper.min_val.y[local_x];
			if (local_y == 0xFF)
				continue;

			map_index = MapGetIndex(local_x, local_y);
			local_temp1 = map[map_index];
			array_value = local_temp1 & (~PREPROCESS_PUSH);
			map[map_index] = array_value;

			++local_y;
			for (; local_y < MAP_SIZE_Y; ++local_y)
			{
				map_index += MAP_SIZE_X;
				check_force();
			}
			map_index = MapGetIndex(local_x, MAP_SIZE_Y - 1);
			local_temp1 = map[map_index];
			array_value = local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN));
			map[map_index] = array_value;

			for (local_y = MAP_SIZE_Y - 2; local_y < MAP_SIZE_Y; --local_y)
			{
				map_index -= MAP_SIZE_X;
				pass_info_back();
			}
		}
		break;
	case DIR_UP:
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			if (preproc_helper.min_val.y[local_x] == 0xFF)
				continue;

			local_y = preproc_helper.max_val.y[local_x];
			map_index = MapGetIndex(local_x, local_y);
			local_temp1 = map[map_index];
			array_value = local_temp1 & (~PREPROCESS_PUSH);
			map[map_index] = array_value;
			--local_y;
			for (; local_y < MAP_SIZE_Y; --local_y)
			{
				map_index -= MAP_SIZE_X;
				check_force();
			}

			map_index = MapGetIndex(local_x, 0);
			local_temp1 = map[map_index];
			array_value = local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN));
			map[map_index] = array_value;

			for (local_y = 1; local_y < MAP_SIZE_Y; ++local_y)
			{
				map_index += MAP_SIZE_X;
				pass_info_back();
			}
		}
		break;
	}


}

// this function goes through objects and according to their properties prepares temp map for quick interactions
void preprocess_move_and_push() // preprocess type can be YOU or MOVE,  depending if the player is moving or other objects
{
	helpers.something_moving = false;
	helpers.something_pushed = false;

	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		local_type = objects.type[local_index];

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		map_index = MapGetIndex(local_x, local_y);
		local_flags = map[map_index];

		lookup_index = obj_prop_lookup[local_type];

		// open/shut flags
		ObjPropGetByIndex(lookup_index, PROP_OPEN, array_value);
		if (array_value)
			local_flags |= PREPROCESS_OPEN;
		ObjPropGetByIndex(lookup_index, PROP_SHUT, array_value);
		if (array_value)
			local_flags |= PREPROCESS_SHUT;

		// move object if going into current direction and has no DIR_CHANGED flag set (ignore other flags)		
		ObjPropGetByIndex(lookup_index, preprocess_type, local_temp1);
		ObjPropGetByIndex(lookup_index, PROP_IRON, local_temp2);
		if ( (local_temp1 && (objects.direction[local_index] & (DIR_MASK | DIR_CHANGED) ) == move_direction ) ||
			 ( (local_flags & PREPROCESS_MAGNET) && local_temp2)
			)
		{
			local_flags |= PREPROCESS_MOVING;
			helpers.something_moving = true;		
			SetMinMaxHelper(local_x, local_y); // uses array_index
		}
		else 
		{
			ObjPropGetByIndex(lookup_index, PROP_PUSH, array_value);
			if (array_value)
				local_flags |= PREPROCESS_PUSH;
		}
		ObjPropGetByIndex(lookup_index, PROP_STOP, array_value);
		if (array_value)
		{
			local_flags |= PREPROCESS_STOP;
		}

		map[map_index] = local_flags;
	}
}

// Additional data for magnet processing optimization
byte magnets_end;
byte magnet_index[MAX_OBJECTS];
byte magnet_direction[MAX_OBJECTS];

void preprocess_magnets()
{
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		local_temp1 = objects.type[local_index];
		ObjPropGet(local_temp1, PROP_MAGNET, array_value);
		if (array_value==false)
			continue;
		// otherwise, it's magnet

		// usually we don't have that many KILLED up to last_obj_index so this one is moved to be second condition
		if (IS_KILLED(local_index))
			continue;

		// local_flags is magnet direction
		magnet_index[magnets_end] = local_index;

		array_value = objects.direction[local_index] & DIR_MASK;
		magnet_direction[magnets_end] = array_value;
		++magnets_end;
	}
}

void cast_magnet_rays()
{
	byte reverted_move_direction = reverted_direction_lookup[move_direction];
	for (local_temp1 = 0; local_temp1 < magnets_end; ++local_temp1)
	{
		local_flags = magnet_direction[local_temp1];
		if (local_flags != reverted_move_direction)
			continue;
		local_index = magnet_index[local_temp1];
		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		map_index = MapGetIndex(local_x, local_y);
		// for speed we have specialized processing of magnet ray casting into different directions
		switch (local_flags)
		{
		case DIR_DOWN:
			for (;;)
			{
				++local_y;
				map_index += MAP_SIZE_X;
				if (local_y >= MAP_SIZE_Y)
					break;
				map[map_index] = PREPROCESS_MAGNET;
			}
			break;
		case DIR_LEFT:
			for (;;)
			{
				--local_x;
				--map_index;
				if (local_x >= MAP_SIZE_X)
					break;
				map[map_index] = PREPROCESS_MAGNET;
			}
			break;
		case DIR_RIGHT:
			for (;;)
			{
				++local_x;
				++map_index;
				if (local_x >= MAP_SIZE_X)
					break;
				map[map_index] = PREPROCESS_MAGNET;
			}
			break;
		case DIR_UP:
			for (;;)
			{
				--local_y;
				map_index -= MAP_SIZE_X;
				if (local_y >= MAP_SIZE_Y)
					break;
				map[map_index] = PREPROCESS_MAGNET;
			}
			break;
		}
	}
}

void move_ok_ones()
{
	byte prop1, prop2;

	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		local_type = objects.type[local_index];
		lookup_index = obj_prop_lookup[local_type];
		// object can move ONLY if it's MOVE, PUSH on PREPROCESS_PUSH or IRON on PREPROCESS_MAGNET
		ObjPropGetByIndex(lookup_index, PROP_IRON, array_value);
		ObjPropGetByIndex(lookup_index, PROP_PUSH, prop1);
		ObjPropGetByIndex(lookup_index, preprocess_type, prop2);
		if (array_value == 0 && prop1 == 0 && prop2 == 0)
			continue;

		// skip if killed
		if (IS_KILLED(local_index))
			continue;

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		MapGet(local_x, local_y, local_temp1);


		if (
			 (local_temp1 & PREPROCESS_MAGNET) && array_value ||
			 (local_temp1 & PREPROCESS_PUSH) && prop1 ||
			 ( prop2 && ( (objects.direction[local_index] & DIR_MASK) == move_direction) )
		   )
		{
			ObjPropGetByIndex(lookup_index, PROP_OPEN, array_value);
			ObjPropGetByIndex(lookup_index, PROP_SHUT, prop1);

			if (
					/* if FINISH, then always can move*/
					local_temp1 & PREPROCESS_MOVE_ACCEPTED ||
						/* if MOVING but it's blocked, then can only move if has paired OPEN/SHUT*/
					(
						(local_temp1 & PREPROCESS_MOVING &&
							(
								((local_temp1 & PREPROCESS_OPEN) && array_value) ||
								((local_temp1 & PREPROCESS_SHUT) && prop1)
							)
						)
					)					
				)			
			{
				// check if text
				if (local_type == TYPE_TEXT || obj_is_word[local_type])
					helpers.rules_may_have_changed = true;

				array_value = move_direction_lookup_x[move_direction];
				objects.x[local_index] += array_value;
				array_value = move_direction_lookup_y[move_direction];
				objects.y[local_index] += array_value;

				objects.direction[local_index] = move_direction;
				helpers.something_moving = true;

				ObjPropGetByIndex(lookup_index, PROP_PUSH, array_value);
				if ((local_temp1 & PREPROCESS_PUSH) && array_value)
					helpers.something_pushed = true;
			}
			else
			{
				// we change direction of moving when hitting STOP, except YOU, because it looks strange
				ObjPropGetByIndex(lookup_index, preprocess_type, array_value);
				ObjPropGetByIndex(lookup_index, PROP_YOU, prop1);
				if (array_value && move_direction == (objects.direction[local_index] & (DIR_MASK | DIR_CHANGED) ) && prop1 == false)
				{
					array_value = reverted_direction_lookup[objects.direction[local_index] & DIR_MASK] | DIR_CHANGED;
					objects.direction[local_index] = array_value;
				}
			}
		}
	}
}

void perform_move()
{
	// clear preprocessed data
	memset(map, PREPROCESS_NONE, sizeof(map));
	clear_preprocess_helper();

	if (preprocess_type == PROP_MOVE && magnets_end > 0)
		cast_magnet_rays();

	preprocess_move_and_push();
	if (helpers.something_moving==false) // this one for optimization
		return;
	apply_force();

	helpers.something_moving = false; // this one for sounds
	move_ok_ones();
	if (helpers.something_pushed)
		audio_sfx(SFX_PUSH);
}

void handle_move()
{
	magnets_end = 0;
	if (rule_exists[PROP_MAGNET] && rule_exists[PROP_IRON])
		preprocess_magnets();

	// move of objects with PROP_MOVE
	move_direction = DIR_RIGHT;
	preprocess_type = PROP_MOVE;
	perform_move();
	move_direction = DIR_UP;
	perform_move();
	move_direction = DIR_LEFT;
	perform_move();
	move_direction = DIR_DOWN;
	perform_move();
}
