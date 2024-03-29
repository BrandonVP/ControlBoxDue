/*
 ===========================================================================
 Name        : Program.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : Class object for holding program position movements
 ===========================================================================
 */

#ifndef _Program_H
#define _Program_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class Program
{
protected:
	uint8_t gripStatus = 2;
	uint16_t wait = 0;
	uint16_t a1 = 0;
	uint16_t a2 = 0;
	uint16_t a3 = 0;
	uint16_t a4 = 0;
	uint16_t a5 = 0;
	uint16_t a6 = 0;
	uint16_t ID = 0;
public:
	Program(uint16_t*, uint8_t, uint16_t, uint16_t);
	uint16_t getA1();
	uint16_t getA2();
	uint16_t getA3();
	uint16_t getA4();
	uint16_t getA5();
	uint16_t getA6();
	uint16_t getID();
	uint16_t getWait();
	uint8_t getGrip();
};

#endif // _Program_H