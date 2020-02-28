#include "headers.h"

/*****************************************************************************/
/*                                   Data                                    */
/*****************************************************************************/

byte level_number; // current level number
byte game_phase; // LEVEL_WON, LEVEL_FAILED etc.

struct game_progress_def game_progress;

// map is used to initiate level objects 
// later it is used to speed up processing of different action handlers or object interactions
byte map[MAP_SIZE_Y*MAP_SIZE_X];

byte you_move_direction = DIR_NONE; // temporary, does not have to be stored

struct preprocess_info_data preproc_helper;
struct helpers_def helpers;

// these are only growing, therefore don't need to be stored for UNDO?
byte last_obj_index; 
byte last_text_index;

//////////////////////// DATA - TO BE STORED FOR UNDO? ////////////////////////

#if defined(__CC65__) && defined(__ATARI__)
// Objects data need to go to non-banked space, because ee store undo data there
#pragma data-name("DATA")
struct objects_def objects = {}; // this way {} we put it into DATA segment
#else
struct objects_def objects; 
#endif


#if defined(__CC65__) && defined(__ATARI__)
#pragma data-name("BANKDATA")
#endif


//////////////////////// END OF DATA TO BE STORED FOR UNDO ////////////////////////

// set by game rules - objects types that has specific property, value tells how many times not to disable if more than once
byte obj_prop[TYPE_MAX * PROPERTY_MAX];

// because TYPE_MAX is 16, we can fit state into uint
uint obj_has[TYPE_MAX]; 

// this is array used during rule parser to treat specific object types differently
bool obj_is_word[TYPE_MAX];

// this is set if property exists on map, to skip unnecessary processing
byte rule_exists[PROPERTY_MAX];

