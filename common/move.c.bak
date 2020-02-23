/*
 Oh boy, making this one was HARD to make it both efficient and working as expected :)
 - Open/Shut has higher priority than STOP
 - Difference from BabaIsYou - Open/Shut is evaluated after move, therefore pushed objects need to wait to enter cleaned space
 - Difference from BabaIsYou - Push has higher priority than Open/Shut, therefore until object can be pushed, it won't open/close
 - Magnets makes IRON objects moving, and moving is used in the apply_force, so magnets have magnet_preprocess. 
   They work through STOP (not like in Atari Robbo but like in real-life), because STOP property is done in other preprocess...
*/
// TODO: 
// 1. Fix magnets - not to be pushed when iron is moving towards it?
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
	local_type = MapGet(local_x, local_y);

	// if previous (to the right in "right to left") is moving
	if ( local_temp1 & PREPROCESS_MOVING)
	{
		// if PUSH then it's moving, however if it's set as FINISH by OPEN/SHUT, then it's not moving
		if ( local_type & PREPROCESS_PUSH && (!(local_type & PREPROCESS_FINISH)) )
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

	MapSet(local_x, local_y, local_type);
	local_temp1 = local_type;
}

void pass_info_back()
{
	local_type = MapGet(local_x, local_y);

	// storing these flags temporary to pass to the next check_force as previous flags 
	local_flags = local_type & (PREPROCESS_OPEN | PREPROCESS_SHUT);

	if (local_type & PREPROCESS_OPEN)
	{
		if (!(local_temp1 & PREPROCESS_SHUT))
			local_type &= (~PREPROCESS_OPEN);
	}
	if (local_type & PREPROCESS_SHUT)
	{
		if (!(local_temp1 & PREPROCESS_OPEN))
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
	MapSet(local_x, local_y, local_type);
	local_temp1 = local_type | local_flags;
}

void apply_force()
{
	switch (move_direction)
	{
	case DIR_LEFT:
		for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
		{
			if (!preproc_helper.preprocess_object_exists_y[local_y])
				continue;

			local_temp1 = MapGet(MAP_SIZE_X-1, local_y);
			// first one is for sure not pushed, so we remove this flag from map
			MapSet(MAP_SIZE_X - 1, local_y, local_temp1 & (~PREPROCESS_PUSH));

			for (local_x = MAP_SIZE_X - 2; local_x < MAP_SIZE_X; --local_x)
			{
				check_force();
			}
			local_temp1 = MapGet(0, local_y);
			MapSet(0, local_y, local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN)));

			for (local_x = 1; local_x < MAP_SIZE_X; ++local_x)
			{
				pass_info_back();
			}
		}
		break;
	case DIR_RIGHT:
		for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
		{
			if (!preproc_helper.preprocess_object_exists_y[local_y])
				continue;

			local_temp1 = MapGet(0, local_y);
			MapSet(0, local_y, local_temp1 & (~PREPROCESS_PUSH) );

			for (local_x = 1; local_x < MAP_SIZE_X; ++local_x)
			{
				check_force();
			}
			local_temp1 = MapGet(MAP_SIZE_X - 1, local_y);
			MapSet(MAP_SIZE_X - 1, local_y, local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN)));

			for (local_x = MAP_SIZE_X - 2; local_x < MAP_SIZE_X; --local_x)
			{
				pass_info_back();
			}
		}
		break;
	case DIR_DOWN:
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			if (!preproc_helper.preprocess_object_exists_x[local_x])
				continue;

			local_temp1 = MapGet(local_x, 0);
			MapSet(local_x, 0, local_temp1 & (~PREPROCESS_PUSH));

			for (local_y = 1; local_y < MAP_SIZE_Y; ++local_y)
			{
				check_force();
			}
			local_temp1 = MapGet(local_x, MAP_SIZE_Y - 1);
			MapSet(local_x, MAP_SIZE_Y - 1, local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN)));

			for (local_y = MAP_SIZE_Y - 2; local_y < MAP_SIZE_Y; --local_y)
			{
				pass_info_back();
			}
		}
		break;
	case DIR_UP:
		for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
		{
			if (!preproc_helper.preprocess_object_exists_x[local_x])
				continue;

			local_temp1 = MapGet(local_x, MAP_SIZE_Y - 1);
			MapSet(local_x, MAP_SIZE_Y - 1, local_temp1 & (~PREPROCESS_PUSH));

			for (local_y = MAP_SIZE_Y - 2; local_y < MAP_SIZE_Y; --local_y)
			{
				check_force();
			}
			local_temp1 = MapGet(local_x, 0);
			MapSet(local_x, 0, local_temp1 & ((~PREPROCESS_SHUT) & (~PREPROCESS_OPEN)));

			for (local_y = 1; local_y < MAP_SIZE_Y; ++local_y)
			{
				pass_info_back();
			}
		}
		break;
	}


}

// this function goes through objects and according to their properties prepares temp map for quick interactions
void preprocess_move_and_push(byte preprocess_type) // preprocess type can be YOU or MOVE,  depending if the player is moving or other objects
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
		local_flags = MapGet(local_x, local_y);

		// open/shut flags
		if (ObjPropGet(local_type, PROP_OPEN))
			local_flags |= PREPROCESS_OPEN;
		if (ObjPropGet(local_type, PROP_SHUT))
			local_flags |= PREPROCESS_SHUT;

		// move object if going into current direction and has no DIR_CHANGED flag set (ignore other flags)
		if ( (ObjPropGet(local_type, preprocess_type) && (objects.direction[local_index] & (DIR_MASK | DIR_CHANGED) ) == move_direction ) ||
			 ( (local_flags & PREPROCESS_MAGNET) && ObjPropGet(local_type, PROP_IRON))
			)
		{
			local_flags |= PREPROCESS_MOVING;
			helpers.something_moving = true;
			preproc_helper.preprocess_object_exists_x[local_x] = true;
			preproc_helper.preprocess_object_exists_y[local_y] = true;
		}
		else if (ObjPropGet(local_type, PROP_PUSH))
		{
			local_flags |= PREPROCESS_PUSH;
		}
		if (ObjPropGet(local_type, PROP_STOP))
		{
			local_flags |= PREPROCESS_STOP;
		}

		MapSet(local_x, local_y, local_flags);
	}
}

void preprocess_magnets()
{
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (!ObjPropGet(objects.type[local_index], PROP_MAGNET))
			continue;

		// usually we don't have that many KILLED up to last_obj_index so this one is moved to be second condition
		if (IS_KILLED(local_index))
			continue;

		// local_flags is magnet direction
		local_flags = objects.direction[local_index] & DIR_MASK;

		if (move_direction != reverted_direction_lookup[local_flags])
			continue;

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];

		local_temp1 = move_direction_lookup_x[local_flags];
		local_temp2 = move_direction_lookup_y[local_flags];

		while(1) 
		{
			local_x += local_temp1;
			local_y += local_temp2;
			if (local_x >= MAP_SIZE_X || local_y >= MAP_SIZE_Y)
				break;
			MapSet(local_x,local_y, PREPROCESS_MAGNET)
		}
	}
}

void move_ok_ones(byte preprocess_type)
{
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		local_type = objects.type[local_index];
		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		local_temp1 = MapGet(local_x, local_y);
		if (
			((local_temp1 & PREPROCESS_MAGNET) && ObjPropGet(local_type, PROP_IRON)) ||
			 (local_temp1 & PREPROCESS_PUSH) && (ObjPropGet(local_type, PROP_PUSH) ) ||
			 ( ObjPropGet(local_type, preprocess_type) && ( (objects.direction[local_index] & DIR_MASK) == move_direction) )
			)
		{

			if (
					/* if FINISH, then always can move*/
					local_temp1 & PREPROCESS_MOVE_ACCEPTED ||
						/* if MOVING but it's blocked, then can only move if has paired OPEN/SHUT*/
					(
						(local_temp1 & PREPROCESS_MOVING &&
							(
								((local_temp1 & PREPROCESS_OPEN) && ObjPropGet(local_type, PROP_OPEN)) ||
								((local_temp1 & PREPROCESS_SHUT) && ObjPropGet(local_type, PROP_SHUT))
							)
						)
					)					
				)			
			{
				// check if text
				if (local_type == TYPE_TEXT || obj_is_word[local_type])
					helpers.rules_may_have_changed = true;
				objects.x[local_index] += move_direction_lookup_x[move_direction];
				objects.y[local_index] += move_direction_lookup_y[move_direction];
				objects.direction[local_index] = move_direction;
				helpers.something_moving = true;
				if ((local_temp1 & PREPROCESS_PUSH) && ObjPropGet(local_type, PROP_PUSH))
					helpers.something_pushed = true;
			}
			else
			{
				// we change direction of moving when hitting STOP, except YOU, because it looks strange
				if (ObjPropGet(local_type, preprocess_type) && move_direction == objects.direction[local_index] && !ObjPropGet(local_type, PROP_YOU))
				{
					objects.direction[local_index] = reverted_direction_lookup[objects.direction[local_index] & DIR_MASK] | DIR_CHANGED;
				}
			}
		}
	}
}

void perform_move(byte preprocess_type)
{
	// clear preprocessed data
	memset(map, PREPROCESS_NONE, sizeof(map));
	clear_preprocess_helper();

	if (preprocess_type == PROP_MOVE && (rule_exists[PROP_MAGNET] && rule_exists[PROP_IRON]))
		preprocess_magnets();

	preprocess_move_and_push(preprocess_type);
	if (!helpers.something_moving) // this one for optimization
		return;
	apply_force();

	helpers.something_moving = false; // this one for sounds
	move_ok_ones(preprocess_type);
	if (helpers.something_pushed)
		audio_sfx(SFX_PUSH);
}

void handle_move()
{
	// move of objects with PROP_MOVE
	move_direction = DIR_RIGHT;
	perform_move(PROP_MOVE);
	move_direction = DIR_UP;
	perform_move(PROP_MOVE);
	move_direction = DIR_LEFT;
	perform_move(PROP_MOVE);
	move_direction = DIR_DOWN;
	perform_move(PROP_MOVE);
}
