#include <stdio.h>
#include <atari.h>
#include <string.h>
#include "ram_handler.h"
#include "../../common/definitions.h"

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

byte undo_index_end;
// the first we can return to
byte undo_index_start;
byte undo_index_max;
bool undo_buffer_crossed;

void store_state()
{
	//printf("STOR: End:%u, RD:%u, WT:%u\n", (unsigned int) undo_index_end, (unsigned int) undo_read_index, (unsigned int) undo_write_index);

	if (undo_index_end == 0)
	{
		undo_read_index = 0; // set current read in a way that the next write will be to 0	
		undo_write_index = 0;
		undo_index_end = 1;
	}
	else
	{
		// extend the buffer up to available space
		if (undo_index_end != undo_index_max)
			++undo_index_end;

		// store data in the next position after the current read position
		undo_write_index = undo_read_index;
		++undo_write_index;
		if (undo_write_index == undo_index_end)
			undo_write_index = 0;
	}		
	write_to_bank();
	// we can now read from this one
	undo_read_index = undo_write_index;		
}

void restore_state()
{
	//printf("REST: End:%u, RD:%u, WT:%u\n", (unsigned int) undo_index_end, (unsigned int) undo_read_index, (unsigned int) undo_write_index);
	// there is nothing to restore
	if (undo_index_end==0)
		return;
	
	read_from_bank();
}

bool undo()
{
	byte prev_index;
	//puts("UNDO\n");
	// there is nothing to restore
	if (undo_index_end==0)
		return false;	

	prev_index = undo_read_index;
	if (undo_read_index==0)
		undo_read_index = undo_index_end-1;
	else
		--undo_read_index;
	
	if (undo_read_index == undo_write_index)
	{
		undo_read_index = prev_index;
		return false;
	}
	restore_state();
	return true;
}

bool redo()
{
	//printf("REDO\n");
	// there is nothing to restore
	if (undo_index_end==0)
		return false;
	if (undo_read_index == undo_write_index)
	{
		//printf("NOTHING TO REDO\n");
		return false;
	}

	++undo_read_index;
	if (undo_read_index>=undo_index_end)
		undo_read_index = 0;
	
	restore_state();
	return true;
}

void reset_undo()
{
	// this could be assigned only once
	undo_index_max = memory_objects_in_all_banks(sizeof(objects));
	undo_index_end = 0; // nothing is stored
	undo_index_start = 0;
}
