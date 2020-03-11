// We mix here 2 different assemblers (MADS and CA65) therefore we merge the created executable blocks
#include <atari.h>
#include <stdio.h>
#include <peekpoke.h>
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
bool initialized=false;
byte audio_volume;

void wait_for_vblank();

void play_sfx(unsigned char effect_number)
{
	if (effect_number!=0)
		POKE(SFX_SFX_EFFECT,effect_number);
}

#define FADE_STEP 0x8

void stop_music()
{
	if (initialized)
	{
		wait_for_vblank();
		currently_playing_track = EMPTY_TRACK;
		POKE(SFX_MUSIC_PLAYS,0);
		__asm__ ("jsr %w",SFX_RMT_SILENCE);
		__asm__ ("jsr %w",SFX_STOP);
		wait_for_vblank();
		initialized = false;
	}
}

void pause_music()
{
	if (currently_playing_track != EMPTY_TRACK)
	{
		for (audio_volume = 0; audio_volume < (255 - FADE_STEP); audio_volume += FADE_STEP)
		{
			POKE(SFX_RMTGLOBALVOLUMEFADE, audio_volume);
			wait_for_vblank();
		}
	}

	POKE(SFX_MUSIC_PLAYS, 0);
	__asm__("jsr %w", SFX_RMT_SILENCE);
	__asm__("jsr %w", SFX_STOP); // disable DLI
}

void continue_music()
{
	__asm__("jsr %w", SFX_RMT_SILENCE);
	__asm__("jsr %w", SFX_SETPOKEY);
	POKE(SFX_MUSIC_PLAYS, 1);
	POKE(SFX_RMTGLOBALVOLUMEFADE, 0xFF);
	__asm__("jsr %w", SFX_ENABLE_VBI);
	if (currently_playing_track != EMPTY_TRACK)
	{
		for (audio_volume = 0xFF; audio_volume > FADE_STEP; audio_volume -= FADE_STEP)
		{
			POKE(SFX_RMTGLOBALVOLUMEFADE, audio_volume);
			wait_for_vblank();
		}
	}
	wait_for_vblank();
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
	POKE(SFX_MUSIC_PLAYS, 1);
	POKE(SFX_TRACK_TO_PLAY,track_number);
	/* Initalize Pokey after disk I/O - this is done by RMT itself
	__asm__("LDA #0");
	__asm__("STA $D208");
	__asm__("LDA #3");
	__asm__("STA $D20F");
	*/
	__asm__("jsr %w", SFX_RMT_SILENCE);
	//wait_for_vblank();
	__asm__("jsr %w", SFX_SETPOKEY);
	//wait_for_vblank();
	__asm__ ("jsr %w",SFX_START);
	wait_for_vblank();
}

void init_sfx()
{
	if (!initialized)
	{
		initialized = true;
		__asm__ ("jsr %w",SFX_NEW_INIT);
		POKE(SFX_MUSIC_PLAYS,0);
		wait_for_vblank();
	}
}


#pragma optimize (on)
