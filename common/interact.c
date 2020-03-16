// This file handles interactions of objects on the same cell, after move
// OPEN/SHUT interaction happens AFTER move, therefore if during the move object is losing one of these properties (e.g. KEY IS WORD), 
// it won't interact, so object can pass through closed door.
#include "extern.h"

#define INTERACT_NONE 0x0
#define INTERACT_YOU  0x1
#define INTERACT_SINK 0x2
#define INTERACT_KILL 0x4
#define INTERACT_OPEN 0x8

#define INTERACT_SHUT		0x10
#define INTERACT_NON_SINK	0x20
#define INTERACT_STOP		0x40
#define INTERACT_ACID	    0x80

byte create_direction;

// Create object at (x=local_temp1,y=local_temp2) of specific type
void create_object(byte type)
{
	// find not set object (local_text_type is not used anywhere so we use it as index)
	for (local_text_type = 0; local_text_type < MAX_OBJECTS; ++local_text_type)
	{
		if (IS_KILLED(local_text_type)==false)
			continue;
		objects.type[local_text_type] = type;

		// check if text
		if (type == TYPE_TEXT)
		{
			if (local_text_type >= last_text_index)
				last_text_index = local_text_type + 1;

			// created TEXT should go through all interactions
			if (last_text_index > last_obj_index)
				last_obj_index = last_text_index;
		}
		ObjPropGet(type, PROP_PICK, array_value);
		if (array_value)
			++helpers.pick_exists_as_object;

		objects.text_type[local_text_type] = local_type;

		objects.x[local_text_type] = local_temp1;
		objects.y[local_text_type] = local_temp2;

		array_value = create_direction | DIR_CREATED;
		objects.direction[local_text_type] = array_value;

		// move last_obj_index
		if (local_text_type >= last_obj_index)
			last_obj_index = local_text_type + 1;

		// always when object is created we need to do parsing of rules, because in this phase we do transformation of object to object
		// if this transformation is moved elsewhere, this would be only needed if (type == TYPE_TEXT)
		helpers.rules_may_have_changed = true;
		return;
	}
}

void teleport()
{
	// find current TELE object
	for (local_temp1 = 0; local_temp1 < last_obj_index; ++local_temp1)
	{
		map_index = objects.type[local_temp1]; // temporary move to map_index to optimize ObjPropGet
		ObjPropGet(local_text_type, PROP_TELE, array_value);
		if (array_value==false || IS_KILLED(local_temp1))
			continue;

		// local_text_type is index of current TELE object
		local_text_type = local_temp1;

		if (objects.x[local_temp1] == local_x && objects.y[local_temp1] == local_y)
		{
			// find new TELE on different location
			do
			{
				++local_temp1;
				if (local_temp1 == last_obj_index)
					local_temp1 = 0;

				map_index = objects.type[local_temp1]; // temporary move to map_index to optimize ObjPropGet
				ObjPropGet(map_index, PROP_TELE, array_value);

				if (array_value == false || IS_KILLED(local_temp1))
					continue;

				if (objects.x[local_temp1] == local_x && objects.y[local_temp1] == local_y)
					continue;

				// done!
				array_value = objects.x[local_temp1];
				objects.x[local_index] = array_value;

				array_value = objects.y[local_temp1];
				objects.y[local_index] = array_value;

				if (local_type == TYPE_TEXT || obj_is_word[local_type])
				{
					helpers.rules_may_have_changed = true;
				}
				audio_sfx(SFX_TELE);
				return;

			} while (local_temp1 != local_text_type);
		}
	}

}

// creates new bang object on localtion (local_temp1, local_temp2)
void create_bang()
{
	MapGet(local_temp1, local_temp2, array_value);
	if (array_value & INTERACT_STOP)
		return;

	create_object(TYPE_BANG);
}

void kill()
{
	// we don't have zero page local_XXX variable free, but speed is not necessary here
	byte has_type;

	create_direction = objects.direction[local_index];
	objects.direction[local_index] = DIR_KILLED;

	// check if text
	if (local_type == TYPE_TEXT || obj_is_word[local_type])
	{
		helpers.rules_may_have_changed = true;
	}

	ObjPropGet(local_type, PROP_PICK, array_value);
	if (array_value)
		--helpers.pick_exists_as_object;

	// process HAS
	for (has_type = 0; has_type < TYPE_MAX; ++has_type)
	{
		if (obj_has[local_type] & (one_lshift_lookup[has_type]))
		{
			local_temp1 = local_x;
			local_temp2 = local_y;
			create_object(has_type);
			helpers.something_transformed = true;
		}
	}

	// check BOOM
	ObjPropGet(local_type, PROP_BOOM, array_value);
	if (array_value)
	{
		helpers.something_exploded = true;
		// up and down
		local_temp1 = local_x;

		/*
		// here
		local_temp2 = local_y;
		create_bang();
		*/

		// up
		// local_temp1 is already set
		if (local_y > 0)
		{
			local_temp2 = local_y - 1;
			create_bang();
		}
		// down
		if (local_y < MAP_SIZE_Y - 1)
		{
			local_temp2 = local_y + 1;
			create_bang();
		}

		// left and right
		local_temp2 = local_y;
		// right
		if (local_x < MAP_SIZE_X - 1)
		{
			local_temp1 = local_x + 1;
			create_bang();
		}
		// left
		if (local_x > 0)
		{
			local_temp1 = local_x - 1;
			create_bang();
		}
	}
}

#define PREPROCESS_TELE  0x1

void process_teleports()
{
	memset(map, INTERACT_NONE, sizeof(map));
	// preprocess tele 
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		// disable CREATED flag in direction and skip interaction for this one in this turn
		if (IS_CREATED(local_index))
			continue;

		// set on map properties

		local_type = objects.type[local_index];
		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		ObjPropGet(local_type, PROP_TELE, array_value);
		if (array_value)
			MapSet(local_x, local_y, PREPROCESS_TELE);
	}
	// teleport objects
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		// disable CREATED flag in direction and skip interaction for this one in this turn
		if (IS_CREATED(local_index))
			continue;


		local_type = objects.type[local_index];
		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		MapGet(local_x, local_y, local_flags);
		ObjPropGet(local_type, PROP_TELE, array_value);
		if ((local_flags & PREPROCESS_TELE) && !array_value)
		{
			teleport();
			continue;
		}
	}

}

void preprocess_interactions()
{
	// preprocess
	helpers.pick_exists_as_object = 0;
	memset(map, INTERACT_NONE, sizeof(map));

	// perform interactions - from TEXTs (0), because TEXT can be destroyed too, 
	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		// disable CREATED flag and DIR_CHANGED flag
		objects.direction[local_index] &= ( (~DIR_CREATED) & (~DIR_CHANGED));
		
		// set on map properties

		local_type = objects.type[local_index];
		local_temp1 = INTERACT_NONE; // interaction flags
		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		MapGet(local_x, local_y, local_flags);

		ObjPropGet(local_type, PROP_YOU, array_value);
		if (array_value)
			local_flags |= INTERACT_YOU;

		ObjPropGet(local_type, PROP_KILL, array_value);
		if (array_value)
			local_flags |= INTERACT_KILL;

		ObjPropGet(local_type, PROP_SHUT, array_value);
		if (array_value)
			local_flags |= INTERACT_SHUT;

		ObjPropGet(local_type, PROP_OPEN, array_value);
		if (array_value)
			local_flags |= INTERACT_OPEN;

		ObjPropGet(local_type, PROP_SINK, array_value);
		if (array_value)
			local_flags |= INTERACT_SINK;
		else
			local_flags |= INTERACT_NON_SINK;

		ObjPropGet(local_type, PROP_STOP, array_value);
		if (array_value)
			local_flags |= INTERACT_STOP;

		ObjPropGet(local_type, PROP_ACID, array_value);
		if (array_value)
			local_flags |= INTERACT_ACID;

		ObjPropGet(local_type, PROP_PICK, array_value);
		if (array_value)
			++helpers.pick_exists_as_object;

		MapSet(local_x, local_y, local_flags);
	}
}

void handle_interactions()
{
	preprocess_interactions();

	for (local_index = 0; local_index < last_obj_index; ++local_index)
	{
		if (IS_KILLED(local_index) || IS_CREATED(local_index))
			continue;

		// process interactions
		local_type = objects.type[local_index];

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];

		// interaction flags
		MapGet(local_x, local_y, local_flags);

		if (local_flags == INTERACT_NONE)
			continue;

		// check WIN and PICK that interact with YOU
		if ((local_flags & INTERACT_YOU))
		{
			// to win all there must be 0 'pick' objects
			ObjPropGet(local_type, PROP_WIN, array_value);
			if ( (helpers.pick_exists_as_object==false) && array_value)
			{
				game_phase = LEVEL_WON;
				break;
			}
			ObjPropGet(local_type, PROP_PICK, array_value);
			if (array_value)
			{
				kill();
				audio_sfx(SFX_PICK);
				continue;
			}
		}

		// check what will destroy the object

		// check SINK
		// if there is non_sink object and this one has sink, remove it

		local_temp1 = false; // to check if object is destroyed
 		ObjPropGet(local_type, PROP_SINK, array_value);
		if ((local_flags & INTERACT_NON_SINK) && array_value)
			local_temp1 = true;
		else
		{
			// if there is sink object and this one has no sink, remove it
			ObjPropGet(local_type, PROP_SINK, array_value);
			if ((local_flags & INTERACT_SINK) && array_value == false)
				local_temp1 = true;
			else
			{
				// check ACID
				ObjPropGet(local_type, PROP_IRON, array_value);
				if ((local_flags & INTERACT_ACID) && array_value)
					local_temp1 = true;
			}
		}

		if (local_temp1 != false)
		{
			kill();
			audio_sfx(SFX_SINK);
			continue;
		}


		// check OPEN
		local_temp1 = false;
		ObjPropGet(local_type, PROP_SHUT, array_value);
		if ((local_flags & INTERACT_OPEN) && array_value)
			local_temp1 = true;
		else
		{
			// check SHUT
			ObjPropGet(local_type, PROP_OPEN, array_value);
			if ((local_flags & INTERACT_SHUT) && array_value)
				local_temp1 = true;
		}

		if (local_temp1 != false)
		{
			kill();
			audio_sfx(SFX_OPEN);
			continue;
		}

		// check KILL (no sound)
		ObjPropGet(local_type, PROP_YOU, array_value);
		if ( (local_flags & INTERACT_KILL) && array_value )
		{
			kill();
			continue;
		}	
	}

	// we don't have space in flags for TELE(PORT) :( therefore we move it to other loop
	// advantage is that this is the last processed step so order of objects does not matter
	if (rule_exists[PROP_TELE])
	{
		process_teleports();
	}
}

