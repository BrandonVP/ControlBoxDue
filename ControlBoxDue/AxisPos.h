// AxisPos.h
#include "Common.h"

#ifndef _AxisPos_h
#define _AxisPos_h
class AxisPos
{
private:
	// Angle values for channel 1
	uint16_t a1c1 = 0;
	uint16_t a2c1 = 0;
	uint16_t a3c1 = 0;
	uint16_t a4c1 = 0;
	uint16_t a5c1 = 0;
	uint16_t a6c1 = 0;
	// Angle values for channel 2
	uint16_t a1c2 = 0;
	uint16_t a2c2 = 0;
	uint16_t a3c2 = 0;
	uint16_t a4c2 = 0;
	uint16_t a5c2 = 0;
	uint16_t a6c2 = 0;
public:
	void drawAxisPos(UTFT);
	void updateAxisPos(CAN_FRAME);
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
