#include "extern.h"

/*
-- In this game parser does not accept multiple IS e.g. "WALL is ROBO is You" or cycle "BABA IS KEKE IS BABA" 
-- TODO: Parser does not allow to have OPERATOR and noun in one place. If this happens, they are considered as invalid (EMPTY)
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

// go through the objects and put related items on the map
void preprocess_game_rules()
{
	clear_preprocess_helper();
	memset(level.map,PREPROCESS_EMPTY,sizeof(level.map));
	
	for (local_index=0;local_index < last_text_index;++local_index)
	{
		if (IS_KILLED(local_index) || obj_type[local_index] != TYPE_TEXT)
			continue;

		local_x = obj_x[local_index];
		local_y = obj_y[local_index];
		MapSet(local_x, local_y, obj_text_type[local_index]);
		preproc_helper.preprocess_object_exists_x[local_x] = true;
		preproc_helper.preprocess_object_exists_y[local_y] = true;
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

								if (obj_type[local_index] == dst_type)
								{
									// if we are changing object to text, make the object type according to original text
									// if we are changing text to text, we keep previous text value
									if (obj_type[local_index] != TYPE_TEXT)
										obj_text_type[local_index] = obj_type[local_index];
									obj_type[local_index] = src_type;
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
					obj_has[dst_type] |= one_lshift_lookup [(src_type & VALUE_MASK)];
				}
			}			
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
	byte text = MapGet(local_x, local_y);
again:
	unexpected_text = false;
	switch (parsing_state)
	{
		case PARSING_STATE_STARTING_EXPRESSION:
		case PARSING_STATE_SET_DST_AND:
			// if not started or Noun+And then we are expecting Noun
			if (IS_NOUN(text)) // only nouns
			{
				// after Noun we need to have operator IS or AND
				parsing_state = PARSING_STATE_SET_DST;
				extracted_dst[extracted_dst_count] = text;
				++extracted_dst_count;
				return;
			}
			else if (parsing_state == PARSING_STATE_SET_DST_AND) 
			{
				unexpected_text = true;
			}
			break;
		case PARSING_STATE_SET_DST:
			if (text == (OPERATOR_IS | AS_OPERATOR))
			{
				// If IS then we are changing to extract SRC
				parsing_state = PARSING_STATE_STARTING_SRC;
				operator_type = OPERATOR_IS;
				return;
			}
			else if (text == (OPERATOR_AND | AS_OPERATOR))
			{
				// with AND we will look for the next Noun
				parsing_state = PARSING_STATE_SET_DST_AND;
				return;
			}
			else if (text == (OPERATOR_HAS | AS_OPERATOR))
			{
				parsing_state = PARSING_STATE_STARTING_SRC;
				operator_type = OPERATOR_HAS;
				return;
			}
			unexpected_text = true;
			break;
		case PARSING_STATE_STARTING_SRC:
		case PARSING_STATE_SET_SRC_AND:
			if (IS_PROPERTY(text) || IS_NOUN(text)) // both Nouns and Props are accepted
			{
				parsing_state = PARSING_STATE_SET_SRC;
				extracted_src[extracted_src_count] = text;
				++extracted_src_count;				
				return;
			}
			unexpected_text = true;
			break;
		case PARSING_STATE_SET_SRC:
			// if we are parsing SRC, then only AND operator is accepted
			if (text == (OPERATOR_AND | AS_OPERATOR))
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

void set_game_rules()
{
	if (!helpers.rules_may_have_changed)
		return;

	helpers.rules_may_have_changed = false;

	// clear rules
	memset(obj_prop,0,sizeof(obj_prop));
	memset(obj_has, 0, sizeof(obj_has));
	memset(rule_exists, false, sizeof(rule_exists));

	// text can be always pushed, but we don't need to add it to rule_exists as it's always processed (for texts)
	ObjPropSet(TYPE_TEXT,PROP_PUSH,true);

	preprocess_game_rules();

	// not needed if initial memory is set properly?
	parsing_state = PARSING_STATE_STARTING_EXPRESSION;
	new_parsing(); 
	
	// parse horizontally
	for (local_y=0;local_y<MAP_SIZE_Y;++local_y)
	{
		if (!preproc_helper.preprocess_object_exists_y[local_y])
			continue;
		for (local_x=0;local_x<MAP_SIZE_X;++local_x)
		{
			parse_next();
		}
		new_parsing();
	}	
	// parse vertically
	for (local_x=0;local_x<MAP_SIZE_X;++local_x)
	{
		if (!preproc_helper.preprocess_object_exists_x[local_x])
			continue;
		for (local_y=0;local_y<MAP_SIZE_Y;++local_y)
		{
			parse_next();
		}
		new_parsing();
	}	
}

