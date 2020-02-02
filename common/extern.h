#ifndef EXTERN_H
#define EXTERN_H

#include "headers.h"

/*****************************************************************************/
/*                                Fast data                                  */
/*****************************************************************************/

// This game is aiming 6502 processor where "zero page" has the fastest access. We are placing the data there when possible
// CC65 does not do "functional call" analysis for variables that could be reused nor if local variables can be reused as function parameters
// therefore we treat C language here as high-level assember and use set of predefined global variables through the whole game
// it allows us to make the code very fast and takes away overhead of passing parameters to functions

// local obj positions
extern byte local_x;
extern byte local_y;
// used to enumarate objects
extern byte local_index;
// local object type
extern byte local_type;
// local text_type if object type is text
extern byte local_text_type;
// local flags, used usually in map preprocess
extern byte local_flags;
// just a local variable
extern byte local_temp1;
extern byte local_temp2;

// these are used by CC65
#ifdef __CC65__
#define refresh()
#pragma zpsym ("local_x")
#pragma zpsym ("local_y")
#pragma zpsym ("local_index")
#pragma zpsym ("local_type")
#pragma zpsym ("local_text_type")
#pragma zpsym ("local_flags")
#pragma zpsym ("local_temp1")
#pragma zpsym ("local_temp2")
#endif

/*****************************************************************************/
/*                                Externs                                    */
/*****************************************************************************/
extern byte game_state; // LLEVEL_WON, LEVEL_FAILED
extern byte level_number;
extern byte you_move_direction;

extern struct game_progress_def game_progress;

extern byte obj_x[MAX_OBJECTS]; // pos
extern byte obj_y[MAX_OBJECTS]; // pos
extern byte obj_type[MAX_OBJECTS];
extern byte obj_text_type[MAX_OBJECTS]; // if obj_type[x] is word then obj_word_type[x] tells what word it is
extern direction obj_direction[MAX_OBJECTS]; // direction object
// objects types that has specific property, value tells how many times not to disable if more than once
extern byte obj_prop[TYPE_MAX*PROPERTY_MAX];
extern uint obj_has[TYPE_MAX];
extern byte rule_exists[PROPERTY_MAX];

extern struct level_set_header_def level_set_header;
extern struct level_def level; // current level
extern struct preprocess_info_data preproc_helper;
extern struct helpers_def helpers;

extern byte last_obj_index;
extern byte last_text_index;

/*****************************************************************************/
/*                                Lookups                                    */
/*****************************************************************************/

extern byte move_direction_lookup_x[5];
extern byte move_direction_lookup_y[5];
extern byte map_lookup[MAP_SIZE_Y];
extern byte obj_prop_lookup[TYPE_MAX];
extern uint one_lshift_lookup[16];
extern byte reverted_direction_lookup[5];

#endif // !EXTERN_H
