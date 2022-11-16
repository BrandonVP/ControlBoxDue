/*
 ===========================================================================
 Name        : Configuration.h
 Author      : Brandon Van Pelt
 Created	 : 11/16/2022
 Description : Configure page
 ===========================================================================
 */

#include "Common.h"

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#ifdef _CONFIGURATION_C

bool loopProgram = true;
bool drawConfig();
void homeArm(uint16_t);
void configButtons();

#else

extern bool loopProgram;
extern bool drawConfig();
extern void homeArm(uint16_t);
extern void configButtons();

#endif // _CONFIGURATION_C
#endif // _CONFIGURATION_H

