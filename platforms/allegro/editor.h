#define ALLEGRO_STATICLINK 1
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>
#include "../../common/definitions.h"

#define TEXT_MAX 255

typedef void(*click_handler_fn)(struct ui_button *);

#define NO_TILE 0xFF

struct ui_button {
	unsigned int button_id;
	int x;
	int y;
	int width;
	int height;
	char text[TEXT_MAX];
	unsigned char tile_id; // indexed from 0 or NO_TILE
	bool enabled;
	click_handler_fn click_handler;
	bool selected;
	ALLEGRO_COLOR color;
};

typedef struct ui_button ui_button;

// these IDs are not necessary because buttons have own click handles, but different values help with debugging

#define BUTTON_MENU_FIRST   1000
#define BUTTON_MENU_NEW     1000
#define BUTTON_MENU_OPEN    1001
#define BUTTON_MENU_SAVEAS  1002

#define BUTTON_MENU_TEST_LEVEL    1003
#define BUTTON_MENU_EXPORT_LEVEL  1004
#define BUTTON_MENU_IMPORT_LEVEL  1005
#define BUTTON_MENU_CLEAR_LEVEL   1006
#define BUTTON_MENU_SWAP_LEVEL    1007
#define BUTTON_MENU_BUILD_GAME	  1008

// 12 backgrounds + 8 planets + 8 walls + 1 shuttle + 1 exit
#define GALAXY_OBJECTS_MAX (12+8+8+2)

#define BUTTON_LEVEL_FIRST 1500

#define BUTTON_DIR_DOWN     2000
#define BUTTON_DIR_LEFT     2001
#define BUTTON_DIR_UP		2002
#define BUTTON_DIR_RIGHT    2003
#define BUTTON_DIR_NONE     2004

#define BUTTON_COLOR0       3000
#define BUTTON_COLOR1       3001
#define BUTTON_COLOR2       3002
#define BUTTON_COLOR3       3003
#define BUTTON_COLOR4       3004

#define BUTTON_PALETTE_COPY        3100
#define BUTTON_PALETTE_PASTE       3101

#define BUTTON_TILESET1       4000

//#define BUTTON_BOARD        4000

#define BUTTON_MAP_OBJECT_FIRST 5000
#define BUTTON_GALAXY_OBJECT_FIRST 5100

#define BUTTON_PALETTE_FIRST    6000


void save_level(byte level_no);
void select_level();
void select_object_to_draw();
void color_click_handler(ui_button *button);
void tileset_click_handler(ui_button *button);
void direction_click_handler(ui_button *button);
void clear_level_click_handler(ui_button *button);
void swap_level_click_handler(ui_button *button);
void build_game_click_handler(ui_button *button);
void select_color(int new_one);
void clear_level();
void clear_level_set();
void create_level_set();
void copy_palette_click_handler(ui_button *button);
void paste_palette_click_handler(ui_button *button);
