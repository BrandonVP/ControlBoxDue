/*
 ===========================================================================
 Name        : Variables.cpp
 Author      : Brandon Van Pelt
 Created	 :
 Description : Memory saver
 ===========================================================================
 */

#define _VARIABLES_C
#include "Variables.h"

 /* How to use

 * Returns true if able to lock, false if already locked *
 Lock first variable: lockVar8(POS0);
 Lock second variable: lockVar8(POS1);
 Lock third variable lockVar32(POS0);

 * Use locked variables *
 g_var8[POS0] = 0;
 g_var8[POS1] = 2;
 g_var32[POS0] = g_var8[POS0] + g_var8[POS1]

 * Unlock variable when finished *
 Unlock first variable: unlockVar8(POS0);
 Unlock second variable: unlockVar8(POS1);
 Unlock third variable unlockVar8(POS0);

 */

 /*Example Application

 bool error = false;
 (lockVar8(POS0)) ? g_var8[POS0] = 0 : error = lockError(POS0, 8); // Comment with variable purpose
 if (error)
 {
	 DEBUG_ERROR("Error: Variable locked");
 }

 ...
 unlockVar8(POS0);

 */

 // Prints error message for failed lock attempt
bool lockError(uint8_t position, uint8_t size)
{
	char buffer[20];
	sprintf(buffer, "uint%d_t pos %d", size, position);
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	drawErrorMSG2(F("  Lock Error"), buffer, F("already locked!"));
	return true;
}

// Returns true if locked
bool isVar8Unlocked(uint8_t pos)
{
	return ((g_var8Lock) & (1 << (pos)));
}

// Returns true if locked
bool isVar16Unlocked(uint8_t pos)
{
	return ((g_var16Lock) & (1 << (pos)));
}

// Returns true if locked
bool isVar32Unlocked(uint8_t pos)
{
	return ((g_var32Lock) & (1 << (pos)));
}

bool lockVar8(uint8_t lock)
{
	uint8_t oldValue = g_var8Lock;
	if ((g_var8Lock |= (1 << lock)) > oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool unlockVar8(uint8_t unlock)
{
	uint8_t oldValue = g_var8Lock;
	if ((g_var8Lock &= ~(1 << unlock)) < oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool lockVar16(uint8_t lock)
{
	uint8_t oldValue = g_var16Lock;
	if ((g_var16Lock |= (1 << lock)) > oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool unlockVar16(uint8_t unlock)
{
	uint8_t oldValue = g_var16Lock;
	if ((g_var16Lock &= ~(1 << unlock)) < oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool lockVar32(uint8_t lock)
{
	uint8_t oldValue = g_var32Lock;
	if ((g_var32Lock |= (1 << lock)) > oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool unlockVar32(uint8_t unlock)
{
	uint8_t oldValue = g_var32Lock;
	if ((g_var32Lock &= ~(1 << unlock)) < oldValue)
	{
		return true;
	}
	else
	{
		return false;
	}
}