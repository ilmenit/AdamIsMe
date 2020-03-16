#include "extern.h"
#include "main.h"

extern byte move_direction;
extern byte preprocess_type;

void handle_you()
{
	game_get_action();
	// move of objects with PROP_YOU
	if (you_move_direction != DIR_NONE)
	{
		// we change direction of YOU objects before doing anything else, because YOU can be also MAGNET
		for (local_index = 0; local_index < last_obj_index; ++local_index)
		{
			// is KILLED moved to be second condition checked for small performance gain, because we won't have many KILLED up to last_obj_index
			local_temp1 = objects.type[local_index];
			ObjPropGet(local_temp1, PROP_YOU, array_value);
			if (array_value && (IS_KILLED(local_index)==false) )
				objects.direction[local_index] = you_move_direction;
		}
		move_direction = you_move_direction;
		preprocess_type = PROP_YOU;
		perform_move();
		if (helpers.something_moving && !helpers.something_pushed)
			audio_sfx(SFX_YOU_MOVE);
	}
}
