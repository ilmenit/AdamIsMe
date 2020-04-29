#ifndef MAIN_H
#define MAIN_H

//#define MapSet(x,y,a) map[(x)+map_lookup[(y)]]=(a);
//#define MapGet(x,y) map[(x)+map_lookup[(y)]]

// Optimization of CC65 array access
#define MapSet(x,y,a) { array_index = (x)+map_lookup[(y)];\
                       array_value = (a);\
                       map[array_index]=array_value; };
#define MapGet(x,y,r) { array_index = (x)+map_lookup[(y)];\
                       r=map[array_index]; };

#define MapGetIndex(x,y) ((x)+map_lookup[(y)])

//#define ObjPropSet(obj,prop,a) obj_prop[(prop)+obj_prop_lookup[(obj)]]=(a);
//#define ObjPropGet(obj,prop) obj_prop[(prop)+obj_prop_lookup[(obj)]]

#define ObjPropSet(obj,prop,a) { array_index = obj_prop_lookup[(obj)];\
                       array_index += (prop);\
                       array_value = (a);\
                       obj_prop[array_index]=array_value; };
#define ObjPropGet(obj,prop,r) { array_index = obj_prop_lookup[(obj)];\
                       array_index += (prop);\
                       r=obj_prop[array_index]; };

#define ObjPropGetByIndex(index,prop,r) { array_index = (index) + (prop);\
                       r=obj_prop[array_index]; };


void init_new_game();
void init_level();
void level_pass();
void set_game_rules();
void handle_you();
void handle_move();
void handle_interactions();
void perform_move();
bool is_level_complete();
void clear_preprocess_helper();

#endif // !MAIN_H
