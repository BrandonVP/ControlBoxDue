// AxisPos.h
#include "CANBus.h"
#include <UTFT.h>
#ifndef _AxisPos_h
#define _AxisPos_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
class CANBus;

class AxisPos
{
 private:
	 uint16_t ARM1ID = 0x0A0;
	 uint16_t ARM2ID = 0x0B0;

	 bool isArm1 = 0;
	 bool isArm2 = 0;

	 int a1c1 = -1;
	 int a2c1 = -1;
	 int a3c1 = -1;
	 int a4c1 = -1;
	 int a5c1 = -1;
	 int a6c1 = -1;

	 int a1c2 = -1;
	 int a2c2 = -1;
	 int a3c2 = -1;
	 int a4c2 = -1;
	 int a5c2 = -1;
	 int a6c2 = -1;

	 CANBus can1;
	 UTFT LCD;

 public:
	AxisPos();
	void drawAxisPos(UTFT);
	void updateAxisPos();
	void armSearch();
	int getA1C1();
	int getA2C1();
	int getA3C1();
	int getA4C1();
	int getA5C1();
	int getA6C1();
	int getA1C2();
	int getA2C2();
	int getA3C2();
	int getA4C2();
	int getA5C2();
	int getA6C2();
};
#endif

