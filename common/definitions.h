#ifndef DEFINITION_S
#define DEFINITION_S

#include "my_types.h"

#ifdef __CC65__
#define EDITOR_ENABLED 0
#define PLATFORM_ATARI 1
#else
#define EDITOR_ENABLED 1
#define EDITOR_ATARI 1
#endif

// SOUNDS

#define MUSIC_DISABLED 0
#define MUSIC_GALAXY   1
#define MUSIC_LEVEL_1  2
#define MUSIC_LEVEL_2  3
#define MUSIC_LEVEL_3  4
#define MUSIC_SFX_ONLY 5
#define MUSIC_MAX      6
#define MUSIC_LEVEL_MAX 3

#define SFX_NONE       0
#define SFX_CLICK      1
#define SFX_LEVEL_WON  2
#define SFX_LEVEL_LOST 3
#define SFX_LANDING    4
#define SFX_TAKE_OFF   5
#define SFX_YOU_MOVE   6
#define SFX_SINK       7
#define SFX_PICK       8
#define SFX_TELE       9
#define SFX_PUSH       10
#define SFX_BOOM       11
#define SFX_OPEN       12
#define SFX_TRANSFORM  13
#define SFX_SWIRL      14
#define SFX_MAX		   15

// galaxy

#define WORLDS_MAX 8
#define LEVELS_PER_WORLD 8
// +1 because the last one is Galaxy Map
#define LEVELS_MAX ( (WORLDS_MAX*LEVELS_PER_WORLD) + 1)
#define LEVEL_GALAXY (LEVELS_MAX - 1)

#define WORLD_OF_LEVEL_NUMBER() (level_number / LEVELS_PER_WORLD)

// colors
#define COLORS_MAX 5

// tilesets
#define TILESET_MAX 9
#define TILES_MAX 64

// map definition

/// TODO: decide if MAP_SIZE_X should be 16 because of ZX SPECTRUM and NES
#define MAP_SIZE_X 20
#define MAP_SIZE_Y 12

/*
#if (MAP_SIZE_Y * MAP_SIZE_X)  < 255
#define MAX_OBJECTS (MAP_SIZE_Y * MAP_SIZE_X)
#else
#define MAX_OBJECTS (255-1)
#endif
*/
// we are limiting number of objects to make it storable in ext. ram for UNDO
#define MAX_OBJECTS 140

// Direction
#define DIR_DOWN  0
#define DIR_LEFT  1
#define DIR_UP    2
#define DIR_RIGHT 3
#define DIR_NONE  4
// we have place in obj_directions to mark that object has been killed
// such objects are skipped from processing
#define DIR_KILLED 8
// Dir changed is used to set that object during move is changing direction
#define DIR_CHANGED 0x10
// if created this turn, then ignore
#define DIR_CREATED 0x20
// if this object (TEXT) is part of active game RULE (to "highlight" it)
#define DIR_ACTIVE_RULE 0x40
#define DIR_MASK 0x07

////////////////////////////////////////////////////////
// Object types
////////////////////////////////////////////////////////
#define TYPE_ROBO   0x00
#define TYPE_SHIP   0x01
#define TYPE_WALL   0x02
#define TYPE_BOX    0x03
#define TYPE_VOID   0x04
#define TYPE_KEY    0x05
#define TYPE_DOOR   0x06
#define TYPE_BOLT   0x07
#define TYPE_BUSH   0x08
#define TYPE_BAT    0x09
#define TYPE_BOMB   0x0A 
#define TYPE_BANG   0x0B
#define TYPE_IMP    0x0C
#define TYPE_ROCK   0x0D
#define TYPE_COIL   0x0E
#define TYPE_TEXT   0x0F
#define TYPE_MAX    0x10

///////////////////////
// Properties
///////////////////////
#define AS_PROPERTY 0x10
#define PROP_YOU   0x00 
#define PROP_WIN   0x01 
#define PROP_STOP  0x02 
#define PROP_PUSH  0x03 
#define PROP_SINK  0x04 
#define PROP_OPEN  0x05 
#define PROP_SHUT  0x06 
#define PROP_PICK  0x07 
#define PROP_KILL  0x08 
#define PROP_MOVE  0x09 
// robbo game specific for BOMB - on destroy creates BANG around 
#define PROP_BOOM  0x0a 
//  gets destroyed next turn
#define PROP_TELE  0x0b 
//  ACID is melts IRON melts (similarly to HOT & MELT)
#define PROP_ACID  0x0c 
#define PROP_IRON  0x0d 
// Magnets pull IRON
#define PROP_MAGNET  0x0e 
#define PROP_WORD    0x0f 
#define PROPERTY_MAX 0x10

//////////////////////////
// Keywords
//////////////////////////
#define AS_OPERATOR  0x20
#define OPERATOR_IS  0x00
#define OPERATOR_AND 0x01
#define OPERATOR_HAS 0x02
#define OPERATOR_MAX 0x03
#define PREPROCESS_EMPTY		 0x80

#define VALUE_MASK 0x0F

///////////////////////////////
// MACROS to check type of text
////////////////////////////////
#define IS_NOUN(text)  ((text & 0xF0)==0)
#define IS_PROPERTY(text) (text & AS_PROPERTY)
#define IS_OPERATOR(text) (text & AS_OPERATOR)
////////////////////////////////////////////////////////
// MACROS for quick setting or checking of object state
////////////////////////////////////////////////////////
#define IS_KILLED(index) (objects.direction[index] & DIR_KILLED)
#define IS_CREATED(index) (objects.direction[index] & DIR_CREATED)
#define IS_ACTIVE_RULE(index) (objects.direction[index] & DIR_ACTIVE_RULE)
////////////////////////////////////////////////////////
/// LEVEL STATES
////////////////////////////////////////////////////////

#define GAME_WAITING_FOR_ACTION  0

#define LEVEL_ONGOING  1
#define LEVEL_WON      2
#define LEVEL_LOST     3
#define LEVEL_LOAD     4
#define LEVEL_QUIT     5
#define GALAXY_ONGOING  6
#define GALAXY_LANDED   7
#define GALAXY_TRIGGER  8

#define SHUTTLE_IN_SPACE 0xFF

struct helpers_def
{
	bool pick_exists_as_object;
	bool something_moving;
	bool something_pushed;
	bool you_move_at_least_once;
	// reasons why rules may change: text is destroyed, created, moved, transformed, teleported
	bool rules_may_have_changed;
	bool something_was_word;
	bool something_exploded;
	bool something_transformed;
};

// 6502 processor cannot handle well "array of structs" like struct objects[MAX_OBJECTS], but can handle well "struct of arrays"
// You need to use objects.x[index] instead of objects[index].x, but still nicely grouped and readable

struct objects_def
{
	// x and y depending on MAP_SIZE could be encoded in one byte, but having it separated is faster (no need to and + bitshift)
	byte x[MAX_OBJECTS]; // pos
	byte y[MAX_OBJECTS]; // pos

	// TYPE and TEXT_TYPE could also be encoded in one byte, but this is faster
	byte type[MAX_OBJECTS];
	byte text_type[MAX_OBJECTS]; // if obj_type[x] is word then obj_word_type[x] tells what word it is

	direction direction[MAX_OBJECTS]; // direction object + additional state flags
};

struct game_progress_def {
	byte galaxy_x; // position on the galaxy map
	byte galaxy_y; // position on the galaxy map
	byte landed_x; // position on the galaxy map
	byte landed_y; // position on the galaxy map
	byte completed_levels; // how many levels player completed
	byte landed_on_world_number;
	byte worlds_state[WORLDS_MAX]; // each bit in byte represents finished level in world
};

// used for optimization, grouped as struct for cleaning by single memset
struct preprocess_info_data {
	bool preprocess_object_exists_x[MAP_SIZE_X];
	bool preprocess_object_exists_y[MAP_SIZE_Y];
};

#if EDITOR_ATARI || PLATFORM_ATARI
struct atari_tiles_info_def
{
	byte tileset_number;
	byte world_colors[COLORS_MAX];
};
#endif


#endif
