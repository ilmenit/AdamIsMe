#include "my_types.h"
#include "definitions.h"
#include "extern.h"

#ifdef __CC65__
#pragma code-name(push,"BANKCODE")
#pragma data-name(push,"BANKDATA")
#pragma data-name(push,"BANKRODATA")
#pragma bss-name (push,"BANKDATA")
#endif

// we do check here for 12, because we may need different size of lookup table if MAP_SIZE_Y is different
#if MAP_SIZE_Y == 12
byte map_lookup[MAP_SIZE_Y] =
{
	MAP_SIZE_X * 0,
	MAP_SIZE_X * 1,
	MAP_SIZE_X * 2,
	MAP_SIZE_X * 3,
	MAP_SIZE_X * 4,
	MAP_SIZE_X * 5,
	MAP_SIZE_X * 6,
	MAP_SIZE_X * 7,
	MAP_SIZE_X * 8,
	MAP_SIZE_X * 9,
	MAP_SIZE_X * 10,
	MAP_SIZE_X * 11
};
#else
#error define here map_lookup for you MAP_SIZE_Y
#endif

#if TYPE_MAX != 0x10 || PROPERTY_MAX != 0x10
#error TYPE_MAX and PROPERTY_MAX must be 0x10 (also in other places in the game engine)
#endif
byte obj_prop_lookup[TYPE_MAX] =
{
	PROPERTY_MAX * 0x0,
	PROPERTY_MAX * 0x1,
	PROPERTY_MAX * 0x2,
	PROPERTY_MAX * 0x3,
	PROPERTY_MAX * 0x4,
	PROPERTY_MAX * 0x5,
	PROPERTY_MAX * 0x6,
	PROPERTY_MAX * 0x7,
	PROPERTY_MAX * 0x8,
	PROPERTY_MAX * 0x9,
	PROPERTY_MAX * 0xA,
	PROPERTY_MAX * 0xB,
	PROPERTY_MAX * 0xC,
	PROPERTY_MAX * 0xD,
	PROPERTY_MAX * 0xE,
	PROPERTY_MAX * 0xF,
};

byte move_direction_lookup_x[5] = { 0, -1,  0, 1, 0 };
byte move_direction_lookup_y[5] = { 1,  0, -1, 0, 0 };

byte reverted_direction_lookup[5] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_NONE };

uint one_lshift_lookup[16] =
{
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80,
	0x100,
	0x200,
	0x400,
	0x800,
	0x1000,
	0x2000,
	0x4000,
	0x8000,
};
