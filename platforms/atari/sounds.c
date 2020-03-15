// We mix here 2 different assemblers (MADS and CA65) therefore we merge the created executable blocks
#include <atari.h>
#include <stdio.h>
#include "sfx\sfx.h"
#include "sounds.h"
#include "../../common/my_types.h"

#ifdef __CC65__
#pragma code-name(push,"BANKCODE")
#pragma data-name(push,"BANKDATA")
#pragma data-name(push,"BANKRODATA")
#pragma bss-name (push,"BANKDATA")
#endif

// DISABLE OPTIMIZATION!!!, we do some hacks here!
// It will crash if the function get f.e. inlined.

#pragma optimize (off)

#define EMPTY_TRACK 0x22

bool audio_only_sfx;
unsigned char currently_playing_track = 0xFF;
byte audio_volume;

void wait_for_vblank();

void play_sfx(unsigned char effect_number)
{
	MY_POKE(SFX_SFX_EFFECT,effect_number);
}

#define FADE_STEP 0x8

void stop_music()
{
	MY_POKE(SFX_SOUND_ACTIVE, 0); // disables playing music until set
	wait_for_vblank();
	MY_POKE(SFX_CALL_SILENCE, 1);
	MY_POKE(currently_playing_track, EMPTY_TRACK); // disables playing music until set
	MY_POKE(SFX_TRACK_TO_PLAY, EMPTY_TRACK);
}

void pause_music()
{
	// if music is not playing, we don't need to pause it
	if (MY_PEEK(SFX_SOUND_ACTIVE) == 0)
		return;
	if (currently_playing_track != EMPTY_TRACK)
	{
		for (audio_volume = 0; audio_volume < (255 - FADE_STEP); audio_volume += FADE_STEP)
		{
			MY_POKE(SFX_RMTGLOBALVOLUMEFADE, audio_volume);
			wait_for_vblank();
		}
	}
	MY_POKE(SFX_SOUND_ACTIVE, 0); // disables playing music until set
	wait_for_vblank();
	MY_POKE(SFX_CALL_SILENCE, 1);
}

void continue_music()
{
	if (MY_PEEK(SFX_SOUND_ACTIVE) == 1)
		return;
	wait_for_vblank();
	MY_POKE(SFX_CALL_SILENCE, 1); // reinit after disk IO
	wait_for_vblank();
	MY_POKE(SFX_RMTGLOBALVOLUMEFADE, 0xFF);
	MY_POKE(SFX_SOUND_ACTIVE, 1); // disables playing music until set
	if (currently_playing_track != EMPTY_TRACK)
	{
		for (audio_volume = 0xFF; audio_volume > FADE_STEP; audio_volume -= FADE_STEP)
		{
			MY_POKE(SFX_RMTGLOBALVOLUMEFADE, audio_volume);
			wait_for_vblank();
		}
	}
	MY_POKE(SFX_RMTGLOBALVOLUMEFADE, 0);
}


// start playing selected track from the beginning
void play_music(unsigned char track_number)
{
	if (audio_only_sfx)
	{
		track_number = EMPTY_TRACK;
	}
	else if (track_number == currently_playing_track)
		return;

	currently_playing_track = track_number;
	MY_POKE(SFX_SOUND_ACTIVE, 1);
	MY_POKE(SFX_TRACK_TO_PLAY,track_number);
	wait_for_vblank();
}

void init_sfx()
{
	__asm__ ("jsr %w",SFX_NEW_INIT); // makes PAL/NTSC detection, stores the new VBI and enables it
	wait_for_vblank();
}

#pragma optimize (on)
