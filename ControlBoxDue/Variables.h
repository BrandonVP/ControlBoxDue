/*
 ===========================================================================
 Name        : Variables.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : Memory saver
 ===========================================================================
 */

#include "Common.h"

#ifndef _VARIABLES_H
#define _VARIABLES_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#define POS0 0x00
#define POS1 0x01
#define POS2 0x02
#define POS3 0x03
#define POS4 0x04
#define POS5 0x05
#define POS6 0x06
#define POS7 0x07

#ifdef _VARIABLES_C

uint8_t g_var8[8] = { 0,0,0,0,0,0,0,0 };
uint8_t g_var8Lock = 0;
uint16_t g_var16[8] = { 0,0,0,0,0,0,0,0 };
uint8_t g_var16Lock = 0;
uint32_t g_var32[8] = { 0,0,0,0,0,0,0,0 };
uint8_t g_var32Lock = 0;

bool lockError(uint8_t, uint8_t);
bool isVar8Unlocked(uint8_t);
bool isVar16Unlocked(uint8_t);
bool isVar32Unlocked(uint8_t);
bool lockVar8(uint8_t);
bool lockVar16(uint8_t);
bool lockVar32(uint8_t);
bool unlockVar8(uint8_t);
bool unlockVar16(uint8_t);
bool unlockVar32(uint8_t);

#else

extern uint8_t g_var8[8];
extern uint8_t g_var8Lock;
extern uint16_t g_var16[8];
extern uint8_t g_var16Lock;
extern uint32_t g_var32[8];
extern uint8_t g_var32Lock;

extern bool lockError(uint8_t, uint8_t);
extern bool isVar8Unlocked(uint8_t);
extern bool isVar16Unlocked(uint8_t);
extern bool isVar32Unlocked(uint8_t);
extern bool lockVar8(uint8_t);
extern bool lockVar16(uint8_t);
extern bool lockVar32(uint8_t);
extern bool unlockVar8(uint8_t);
extern bool unlockVar16(uint8_t);
extern bool unlockVar32(uint8_t);

#endif // _VARIABLES_C
#endif // _VARIABLES_H
