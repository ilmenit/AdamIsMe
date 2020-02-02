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

struct level_set_header_def
{
	byte magic[4];
	byte number_of_levels;
	byte map_size_x;
	byte map_size_y;
};

#ifndef __CC65__
#pragma pack()
#endif


void load_level();
void decode_level();
void load_level_data();

#endif // !LEVEL_H
