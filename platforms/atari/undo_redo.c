#include <stdio.h>
#include <atari.h>
#include <string.h>
#include "ram_handler.h"
#include "../../common/definitions.h"
#include "../../common/extern.h"

#pragma code-name("CODE")
#pragma data-name("DATA")
#pragma rodata-name("RODATA")
#pragma bss-name ("BSS")

// Undo is implemented as circular buffer
// WARNING, we assume here BIG object to be stored, therefore we use only byte as undo state index
// these should be replaced by unsigned int we we want to make UNDO for more than 255 objects (!)
byte undo_write_index=0;
byte undo_read_index=0;
extern struct objects_def objects;

// these are separated to non-banked memory

void write_to_bank()
{
	memcpy(get_banked_address(undo_write_index, sizeof(objects)), &objects, sizeof(objects));
	// restore original bank that is changed by get_banked_address
	memory_select_bank(BANK_NONE);
}

void read_from_bank()
{
	memcpy(&objects, get_banked_address(undo_read_index, sizeof(objects)), sizeof(objects));
	// restore original bank that is changed by get_banked_address
	memory_select_bank(BANK_NONE);
}

#pragma code-name("BANKCODE")
#pragma data-name("BANKDATA")
#pragma rodata-name("BANKRODATA")
#pragma bss-name ("BANKDATA")

bool undo_something_stored;
// the first we can return to
byte undo_start_index;
byte undo_index_max;
bool undo_buffer_crossed;

void store_state()
{
	if (!undo_something_stored)
	{
		undo_read_index = 0; // set current read in a way that the next write will be to 0	
		undo_write_index = 0;
		undo_start_index = undo_index_max - 1;
		undo_something_stored = true;
	}
	else
	{
		// store data in the next position after the current read position
		undo_write_index = undo_read_index;
		++undo_write_index;
		if (undo_write_index == undo_index_max)
			undo_write_index = 0;
		// write_index is pushing the start_index, therefore we can revert state only to the next after the current write_index
		if (undo_write_index == undo_start_index)
		{
			++undo_start_index;
			if (undo_start_index == undo_index_max)
				undo_start_index = 0;
		}
	}		
	write_to_bank();
	// we can now read from this one
	undo_read_index = undo_write_index;		
}

void restore_state()
{
	// there is nothing to restore
	if (!undo_something_stored)
		return;
	
	read_from_bank();
}

bool undo()
{
	byte prev_index;
	// there is nothing to restore
	if (!undo_something_stored)
		return false;	

	prev_index = undo_read_index;
	if (undo_read_index==0)
		undo_read_index = undo_index_max-1;
	else
		--undo_read_index;
	
	if (undo_read_index == undo_start_index)
	{
		undo_read_index = prev_index;
		return false;
	}
	restore_state();
	return true;
}

bool redo()
{
	// there is nothing to restore
	if (!undo_something_stored)
		return false;
	if (undo_read_index == undo_write_index)
	{
		return false;
	}

	++undo_read_index;
	if (undo_read_index == undo_index_max)
		undo_read_index = 0;
	
	restore_state();
	return true;
}

void reset_undo()
{
	// this could be assigned only once
	undo_index_max = memory_objects_in_all_banks(sizeof(objects));
	undo_something_stored = false; // nothing is stored
}
