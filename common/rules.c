#include "extern.h"

/*
In this game parser does not accept multiple IS e.g. "WALL is ROBO is You" or cycle "BABA IS KEKE IS BABA" nor order of operators 
Reason is simplification of the parser.
-- TODO???: Text stacking? OR not allowing to have OPERATOR and noun in one place. If this happens, they are considered as invalid (EMPTY) (?)
*/


// "DST is SRC" - like in (DST = SRC), where DST is obj and SRC can be obj or property
#define PARSING_STATE_STARTING_EXPRESSION 0
#define PARSING_STATE_SET_DST 1
#define PARSING_STATE_SET_DST_AND 3
#define PARSING_STATE_STARTING_SRC 4
#define PARSING_STATE_SET_SRC 5
#define PARSING_STATE_SET_SRC_AND 6

byte parsing_state;
byte operator_type;
byte extracted_dst_count;
byte extracted_src_count;
byte extracted_dst[MAP_SIZE_X/2]; // there won't be more on screen, because they are separated by operators
byte extracted_src[MAP_SIZE_X/2]; // there won't be more on screen
bool parsing_horizontally;
byte expression_start_position;

// go through the objects and put related items on the map
void preprocess_game_rules()
{
	clear_preprocess_helper();
	memset(map,PREPROCESS_EMPTY,sizeof(map));
	
	for (local_index=0;local_index < last_text_index;++local_index)
	{
		if (IS_KILLED(local_index))
			continue;

		local_type = objects.type[local_index];
		if (local_type == TYPE_TEXT)
			local_temp1 = objects.text_type[local_index];
		else if (obj_is_word[local_type])
			local_temp1 = local_type;
		else
			continue;

		local_x = objects.x[local_index];
		local_y = objects.y[local_index];
		// remove flag IS_ACTIVE_RULE
		objects.direction[local_index] &= (~DIR_ACTIVE_RULE);
		MapSet(local_x, local_y, local_temp1);
		SetMinMaxHelper(local_x, local_y);
	}
}

void new_parsing()
{
	byte dst_type;
	byte src_type;
	// apply rule if we have enough data - we have at least one DST and SRC
	if (parsing_state == PARSING_STATE_SET_SRC || parsing_state == PARSING_STATE_SET_SRC_AND) 
	{
		for (local_temp1=0;local_temp1<extracted_dst_count;++local_temp1)
		{
			for (local_temp2=0;local_temp2<extracted_src_count;++local_temp2)
			{
				dst_type = extracted_dst[local_temp1];
				src_type = extracted_src[local_temp2];
				if (operator_type == OPERATOR_IS)
				{
					if (IS_NOUN(src_type)) // only nouns
					{
						if (src_type != dst_type)
						{
							// change object into other object, ??? but only once per turn???
							for (local_index = 0; local_index < last_obj_index; ++local_index)
							{
								if (IS_KILLED(local_index))
									continue;

								if (objects.type[local_index] == dst_type)
								{
									// if we are changing object to text, make the object type according to original text
									// if we are changing text to text, we keep previous text value
									if (objects.type[local_index] != TYPE_TEXT)
										objects.text_type[local_index] = objects.type[local_index];
									if (src_type == TYPE_TEXT)
									{
										if (local_index >= last_text_index)
											last_text_index = local_index + 1;
									}
									objects.type[local_index] = src_type;
									helpers.something_transformed = true;
									helpers.rules_may_have_changed = true;
								}
							}
						}
					}
					else
					{
						ObjPropSet(dst_type,(src_type & VALUE_MASK), true);
						rule_exists[src_type & VALUE_MASK] = true;
					}
				}
				else // operator_has
				{					
					if (IS_NOUN(src_type)) // HAS works only for objects, to fit in uint
						obj_has[dst_type] |= one_lshift_lookup [(src_type & VALUE_MASK)];
				}
			}			
		}
		// mark on map that this expression is active. operator_type is not used anymore so we can reuse it
		if (parsing_horizontally)
		{
			do {
				MapSet(expression_start_position, local_y, MapGet(expression_start_position, local_y) | DIR_ACTIVE_RULE);
			} while (expression_start_position++ < local_x-1);			
		}
		else
		{
			do {
				MapSet(local_x, expression_start_position, MapGet(local_x, expression_start_position) | DIR_ACTIVE_RULE);
			} while (expression_start_position++ < local_y-1);
		}
	}
	// reset state
	parsing_state = PARSING_STATE_STARTING_EXPRESSION;
	operator_type = OPERATOR_MAX;
	extracted_dst_count=0;
	extracted_src_count=0;			
}


// 8bit machine is too slow for nice FSM implementation so we use simple Switch-If
void parse_next()
{
	bool unexpected_text;
	byte current_text = MapGet(local_x, local_y) & (~DIR_ACTIVE_RULE);
again:
	unexpected_text = false;
	switch (parsing_state)
	{
		case PARSING_STATE_STARTING_EXPRESSION:
			if (parsing_horizontally)
				expression_start_position = local_x;
			else
				expression_start_position = local_y;
		case PARSING_STATE_SET_DST_AND:
			// if not started or Noun+And then we are expecting Noun
			if (IS_NOUN(current_text)) // only nouns
			{
				// after Noun we need to have operator IS or AND
				parsing_state = PARSING_STATE_SET_DST;
				extracted_dst[extracted_dst_count] = current_text;
				++extracted_dst_count;
				return;
			}
			else if (parsing_state == PARSING_STATE_SET_DST_AND) 
			{
				unexpected_text = true;
			}
			break;
		case PARSING_STATE_SET_DST:
			if (current_text == (OPERATOR_IS | AS_OPERATOR))
			{
				// If IS then we are changing to extract SRC
				parsing_state = PARSING_STATE_STARTING_SRC;
				operator_type = OPERATOR_IS;
				return;
			}
			else if (current_text == (OPERATOR_AND | AS_OPERATOR))
			{
				// with AND we will look for the next Noun
				parsing_state = PARSING_STATE_SET_DST_AND;
				return;
			}
			else if (current_text == (OPERATOR_HAS | AS_OPERATOR))
			{
				parsing_state = PARSING_STATE_STARTING_SRC;
				operator_type = OPERATOR_HAS;
				return;
			}
			unexpected_text = true;
			break;
		case PARSING_STATE_STARTING_SRC:
		case PARSING_STATE_SET_SRC_AND:
			if (IS_PROPERTY(current_text) || IS_NOUN(current_text)) // both Nouns and Props are accepted
			{
				parsing_state = PARSING_STATE_SET_SRC;
				extracted_src[extracted_src_count] = current_text;
				++extracted_src_count;				
				return;
			}
			unexpected_text = true;
			break;
		case PARSING_STATE_SET_SRC:
			// if we are parsing SRC, then only AND operator is accepted
			if (current_text == (OPERATOR_AND | AS_OPERATOR))
			{
				parsing_state = PARSING_STATE_SET_SRC_AND;
				return;				
			}
			else
			{
				unexpected_text = true;
			}
			break;
	}
	if (unexpected_text)
	{
		new_parsing();
		goto again;										
	}
}


void do_parsing()
{
	// not needed if initial memory is set properly
	parsing_state = PARSING_STATE_STARTING_EXPRESSION;
	new_parsing();

	// parse horizontally
	parsing_horizontally = true;
	for (local_y = 0; local_y < MAP_SIZE_Y; ++local_y)
	{
		// there is nothing in this line
		local_x = preproc_helper.min_val.x[local_y];
		if (local_x == 0xFF)
			continue;
		for (; local_x <= preproc_helper.max_val.x[local_y]; ++local_x)
		{
			parse_next();
		}
		new_parsing();
	}
	// parse vertically
	parsing_horizontally = false;
	for (local_x = 0; local_x < MAP_SIZE_X; ++local_x)
	{
		// there is nothing in this column
		local_y = preproc_helper.min_val.y[local_x];
		if (local_y == 0xFF)
			continue;
		for (; local_y <= preproc_helper.max_val.y[local_x]; ++local_y)
		{
			parse_next();
		}
		new_parsing();
	}
}

void finalize_parsing()
{
	// mark objects that stay on active cells on map
	for (local_index = 0; local_index < last_text_index; ++local_index)
	{
		if (IS_KILLED(local_index))
			continue;
		if (MapGet(objects.x[local_index], objects.y[local_index]) & DIR_ACTIVE_RULE)
			objects.direction[local_index] |= DIR_ACTIVE_RULE;
		else
			objects.direction[local_index] &= (~DIR_ACTIVE_RULE);
	}

	// copy IS WORD to helper array for the next rules pass
	memset(obj_is_word, false, sizeof(obj_is_word));
	for (local_type = 0; local_type < TYPE_MAX; ++local_type)
	{
		if (ObjPropGet(local_type, PROP_WORD))
		{
			obj_is_word[local_type] = true;
			helpers.rules_may_have_changed = true;
		}
	}
	if (helpers.rules_may_have_changed) // this should be set only if some object IS WORD
	{
		// we also need to parse non-text objects as TEXT from now on
		if (last_text_index < last_obj_index)
			last_text_index = last_obj_index;
	}
	// if something WAS WORD previously and now nothing is WORD, we need to do parsing again
	if (helpers.something_was_word)
		helpers.rules_may_have_changed = true;
}

void set_game_rules()
{
	if (helpers.rules_may_have_changed==false)
		return;

	helpers.rules_may_have_changed = false;

	if (rule_exists[PROP_WORD])
		helpers.something_was_word = true;
	else
		helpers.something_was_word = false;

	// clear rules
	memset(obj_prop,0,sizeof(obj_prop));
	memset(obj_has, 0, sizeof(obj_has));
	memset(rule_exists, false, sizeof(rule_exists));

	// text can be always pushed, but we don't need to add it to rule_exists as it's always processed (for texts)
	ObjPropSet(TYPE_TEXT,PROP_PUSH,true);

	preprocess_game_rules();
	do_parsing();
	finalize_parsing();

}

