#ifndef SOUNDS_H
#define SOUNDS_H

void init_sfx();
void play_sfx(unsigned char effect);
void play_music(unsigned char track_number);
void stop_music();
void pause_music();
void continue_music();

#endif
