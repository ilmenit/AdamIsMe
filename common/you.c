#include "extern.h"
#include "main.h"

extern byte move_direction;

void handle_you()
{
	game_get_action();
	// move of objects with PROP_YOU
	if (you_move_direction != DIR_NONE)
	{
		for (local_index = 0; local_index < last_obj_index; ++local_index)
		{
			if (IS_KILLED(local_index))
				continue;

			local_type = obj_type[local_index];
			if (ObjPropGet(local_type,PROP_YOU))
				obj_direction[local_index] = you_move_direction;
		}
		move_direction = you_move_direction;
		perform_move(PROP_YOU);
		if (helpers.something_moving && !helpers.something_pushed)
			audio_sfx(SFX_YOU_MOVE);
	}
}
