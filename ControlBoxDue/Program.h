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
	 uint8_t a1 = 0;
	 uint8_t a2 = 0;
	 uint8_t a3 = 0;
	 uint8_t a4 = 0;
	 uint8_t a5 = 0;
	 uint8_t a6 = 0;
	 bool grip = 0;
 public:
	Program(uint8_t*, bool);
	uint8_t getA1();
	uint8_t getA2();
	uint8_t getA3();
	uint8_t getA4();
	uint8_t getA5();
	uint8_t getA6();
	bool getGrip();
};

#endif

