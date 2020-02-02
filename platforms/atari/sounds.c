#include <atari.h>
#include <stdio.h>
#include <peekpoke.h>
#include "sfx\sfx.h"
#include "sounds.h"
#include "../../common/my_types.h"

// DISABLE OPTIMIZATION!!!, we do some hacks here!
// It will crash if the function get f.e. inlined.

#pragma optimize (off)

#define EMPTY_TRACK 0x22

bool audio_only_sfx;
unsigned char currently_playing_track = 0xff;
bool initialized=false;

void play_sfx(unsigned char effect_number)
{
	if (effect_number!=0)
		POKE(SFX_SFX_EFFECT,effect_number);
}

void stop_music()
{
	if (initialized)
	{
		initialized = false;
		currently_playing_track = EMPTY_TRACK;
		POKE(SFX_MUSIC_PLAYS,0);
		__asm__ ("jsr %w",SFX_RMT_SILENCE);
		__asm__ ("jsr %w",SFX_STOP);
	}
}

void play_music(unsigned char track_number)
{
	if (!initialized)
		init_sfx();

	if (audio_only_sfx)
	{
		track_number = EMPTY_TRACK;
	}
	else if (track_number == currently_playing_track)
		return;

	currently_playing_track = track_number;
	POKE(SFX_TRACK_TO_PLAY,track_number);
	__asm__ ("jsr %w",SFX_START);
}

void init_sfx()
{
	if (!initialized)
	{
		initialized = true;
		__asm__ ("jsr %w",SFX_START2);
		POKE(SFX_MUSIC_PLAYS,1);
	}
}


#pragma optimize (on)
