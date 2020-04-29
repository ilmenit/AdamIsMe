#ifndef SOUNDS_H
#define SOUNDS_H

// we don't use <peekpoke.h> because PEEK() there returns int value which in case of if(PEEK()) generates terrible code
#define MY_PEEK(n) (*((unsigned char*)n))
#define MY_POKE(n,val) (*((unsigned char*)n)=val)

void init_sfx();
void deinit_sfx();
void play_sfx(unsigned char effect);
void play_music(unsigned char track_number);
void stop_music();
void pause_music();
void continue_music();

#endif
