#define BANK_NONE 0xFF

void memory_handler_init();
unsigned char memory_get_bank_count();

// returns how many objects we can fit into bank
unsigned int memory_objects_in_bank(unsigned int object_size);
unsigned int memory_objects_in_all_banks(unsigned int object_size);

// sets proper memory bank and returns address in passed offset (or NULL if out of available space)
unsigned char *get_banked_address(unsigned int object_index, unsigned int object_size);

// BANK_NONE for disabled bank 
void memory_select_bank(unsigned char bank);
