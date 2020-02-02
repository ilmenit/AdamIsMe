#include "headers.h"

/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/

byte level_number; // current level number
byte game_state; // LEVEL_WON, LEVEL_FAILED etc.

struct game_progress_def game_progress;
struct level_set_header_def level_set_header;
struct level_def level; // currently loaded level

byte you_move_direction = DIR_NONE; // temporary, does not have to be stored

struct preprocess_info_data preproc_helper;
struct helpers_def helpers;

//////////////////////// DATA - TO BE STORED FOR UNDO? ////////////////////////

// Objects are stored in following way:
// - IS, AND words (starting from 0) - they don't change
// - other words
// - objects
// and there are related indexes of them (e.g. first word), IS words start from 0

byte last_obj_index;
byte last_text_index;

byte obj_x[MAX_OBJECTS]; // pos
byte obj_y[MAX_OBJECTS]; // pos
byte obj_type[MAX_OBJECTS];
byte obj_text_type[MAX_OBJECTS]; // if obj_type[x] is word then obj_word_type[x] tells what word it is
direction obj_direction[MAX_OBJECTS]; // direction object

//////////////////////// END OF DATA TO BE STORED FOR UNDO ////////////////////////

// set by game rules - objects types that has specific property, value tells how many times not to disable if more than once
byte obj_prop[TYPE_MAX * PROPERTY_MAX];

// because TYPE_MAX is 16, we can fit state into uint
uint obj_has[TYPE_MAX]; 

// this is set if property exists on map, to skip unnecessary processing
byte rule_exists[PROPERTY_MAX];

