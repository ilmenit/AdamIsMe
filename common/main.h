#ifndef MAIN_H
#define MAIN_H

#define MapSet(x,y,a) map[(x)+map_lookup[(y)]]=(a);
#define MapGet(x,y) map[(x)+map_lookup[(y)]]
#define MapGetIndex(x,y) ((x)+map_lookup[(y)])

#define ObjPropSet(obj,prop,a) obj_prop[(prop)+obj_prop_lookup[(obj)]]=(a);
#define ObjPropGet(obj,prop) obj_prop[(prop)+obj_prop_lookup[(obj)]]

void init_new_game();
void init_level();
void level_pass();
void set_game_rules();
void handle_you();
void handle_move();
void handle_interactions();
void perform_move(byte preprocess_type);
bool is_level_complete();
void clear_preprocess_helper();

#endif // !MAIN_H
