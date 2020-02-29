#include <atari.h>
#include <stdio.h>
#include "ram_handler.h"
#include "../../common/definitions.h"

#define UNDO_NON_EXT_MOVES 4
// ===============================================

#pragma code-name(push,"BANKCODE")
#pragma data-name(push,"BANKDATA")
#pragma rodata-name(push,"BANKRODATA")
#pragma bss-name (push,"BANKDATA")
unsigned char bank_test=0; // we put some variable in BANK area for testing of extended memory
#pragma code-name(pop)
#pragma data-name(pop)
#pragma rodata-name(pop)
#pragma bss-name (pop)

#pragma code-name("CODE")
#pragma data-name("DATA")
#pragma rodata-name("RODATA")
#pragma bss-name ("BSS")

// ===============================================

// In case of extended memory we bank area
#define BANK_ADDRESS       ((unsigned char*)0x4000)
#define BANK_SIZE 0x4000
// now we using only 64KB, therefore 4 banks (Atari 130XE), can be changed for more extended RAM 
#define BANK_MAX 4

unsigned char bank_selection[BANK_MAX] =
{
	PORTB_BANKSELECT1,
	PORTB_BANKSELECT2,
	PORTB_BANKSELECT3,
	PORTB_BANKSELECT4,
};

unsigned char value_in_bank = 0;
unsigned char prev_portb = 0;
unsigned char memory_banks = 0;
byte* bank_ptr = (byte *) 0xD800;
unsigned int bank_size = (0xE400 - 0xD800);

void memory_select_bank(unsigned char bank)
{
	if (bank == BANK_NONE)
	{
		PIA.portb = prev_portb;
	}
	else
	{
		if (memory_banks == 1)
		{
			// disable ROM
			PIA.portb = (prev_portb & 0b11111110);
		}
		else
			PIA.portb = (prev_portb & 0b11000011) | bank_selection[bank];

	}
}

void memory_handler_init()
{
	prev_portb = PIA.portb;
	
	// get value from bank
	memory_select_bank(3);
	value_in_bank = bank_test;
	
	// disable bank and increase the value
	memory_select_bank(BANK_NONE);
	++bank_test;
	
	// enable the bank and check the value
	memory_select_bank(3);	

// if value in the bank is not changed, then we have extended memory!
	if (bank_test == value_in_bank)
	{
		memory_banks = 4;
		bank_size = BANK_SIZE;
		bank_ptr = BANK_ADDRESS;
	}
	else
	{
		memory_banks = 1;
		// bank_size and bank_ptr are predefined
	}
	memory_select_bank(BANK_NONE); // restore the previous bank, because we succesfuly changed it
}

unsigned char *get_banked_address(unsigned int object_index, unsigned int object_size)
{
	unsigned int objects_in_bank;
	unsigned char bank_number;
	objects_in_bank = memory_objects_in_bank(object_size);
	bank_number = (unsigned char) (object_index / objects_in_bank);
	//printf("BANK: %d All:%d\n", (unsigned int) bank_number, (unsigned int) memory_banks);
	if (bank_number >= memory_banks)
		return NULL;
	
	memory_select_bank(bank_number);

	return &bank_ptr[(object_index % objects_in_bank) * object_size];
}

#pragma code-name("BANKCODE")

unsigned int memory_objects_in_bank(unsigned int object_size)
{
	return bank_size / object_size;
}

unsigned int memory_objects_in_all_banks(unsigned int object_size)
{
	return memory_objects_in_bank(object_size) * (unsigned int)memory_banks;
}
