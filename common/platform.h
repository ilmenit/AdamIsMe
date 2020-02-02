#ifndef PLATFORM_H
#define PLATFORM_H

#include "my_types.h"

// Platform specific functions
// What may be surprising, we do not pass parameters to the functions and the reason is that all the data is global and stored in data.c
// for the code-size and speed optimization purposes. Stack operations are slow and take a lot of space on 6502 CPU.
// TODO: document better which global variables are "parameters" of these functions

// platform specific functions
void init_platform();
void audio_music(unsigned char music_id);
void audio_sfx(unsigned char sfx_id);

bool load_game_progress();
void save_game_progress();

/* 
 * \brief: Loads level data to &level
 * \param: level_number
 * \return: &level filled with level data
 * 
 * data of levelcan be stored in file, in ROM or compressed, depending on the platform, so moved to platform dependent
*/
void load_level_data();
void set_tileset();
void set_palette();

// galaxy (map)
void galaxy_draw_screen();
void galaxy_get_action();

// game (during level)
void game_lost();
void game_get_action();
void game_draw_screen();

#endif // !PLATFORM_H

