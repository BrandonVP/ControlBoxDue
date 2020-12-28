// Program.h

#ifndef _Program_h
#define _Program_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class Program
{
 protected:
	 bool gripStatus = true;
	 uint16_t a1 = 0;
	 uint16_t a2 = 0;
	 uint16_t a3 = 0;
	 uint16_t a4 = 0;
	 uint16_t a5 = 0;
	 uint16_t a6 = 0;
	 uint8_t channel = 0;
 public:
	Program(uint16_t*, bool, uint8_t);
	uint16_t getA1();
	uint16_t getA2();
	uint16_t getA3();
	uint16_t getA4();
	uint16_t getA5();
	uint16_t getA6();
	uint8_t getID();
	bool getGrip();
};

#endif

