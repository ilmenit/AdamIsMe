#ifndef LEVEL_H
#define LEVEL_H

#include "definitions.h"

#define DECODE_DIRECTION(input) (input>>6)
#define ENCODE_DIRECTION(input) (input<<6)
#define ENCODING_MASK 0x3F

// we won't have that many operators and on galaxy map this (63) should be empty one
#define LEVEL_DECODE_EMPTY 0x3F

// THESE EQUAL REPRESENTATION ID in GALAXY
#define DECODE_WORLDS_MIN 0x00
#define DECODE_WORLDS_MAX 0x08

#define DECODE_WALLS_MIN  0x08
#define DECODE_WALLS_MAX  0x10

#define DECODE_LEVEL_NUMBERS_MIN 0x10
#define DECODE_LEVEL_NUMBERS_MAX 0x18

#define DECODE_LOCKS_MIN 0x18
#define DECODE_LOCKS_MAX 0x20

// 6 backgrounds
#define DECODE_BACKGROUND_MIN 0x20
#define DECODE_BACKGROUND_MAX 0x25

#define DECODE_SHUTTLE 0x26
#define DECODE_SHUTTLE_LANDED 0x27

#ifndef __CC65__
#pragma pack(1)
#endif

struct game_progress_def {
	byte galaxy_x; // position on the galaxy map
	byte galaxy_y; // position on the galaxy map
	byte landed_x; // position on the galaxy map
	byte landed_y; // position on the galaxy map
	byte completed_levels; // how many levels player completed
	byte landed_on_world_number;
	byte worlds_state[WORLDS_MAX]; // each bit in byte represents finished level in world
};

struct level_def
{
	// used to initiate level or later for preprocessing information
	// helper map to speed up processing of different action handlers or object interactions
	byte map[MAP_SIZE_Y*MAP_SIZE_X];
	byte tileset_number; // if game has more tilesets
	byte level_colors[COLORS_MAX];
};

struct level_set_header_def
{
	byte magic[4];
	byte number_of_levels;
	byte map_size_x;
	byte map_size_y;
};

struct level_set_def
{
	struct level_set_header_def level_set_header;
	struct level_def levels_data[LEVELS_MAX];
};

// used for optimization, grouped as struct for cleaning by single memset
struct preprocess_info_data {
	bool preprocess_object_exists_x[MAP_SIZE_X];
	bool preprocess_object_exists_y[MAP_SIZE_Y];
};

// 
struct helpers_def
{
	bool pick_exists_as_object;
	bool something_moving;
	bool something_pushed;
	bool you_move_at_least_once;
	// reasons why rules may change: text is destroyed, created, moved, transformed, teleported
	bool rules_may_have_changed; 
	bool something_exploded;
	bool something_transformed;
};

#ifndef __CC65__
#pragma pack()
#endif


void load_level();
void decode_level();
void load_level_data();

#endif // !LEVEL_H
