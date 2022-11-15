/*
 ===========================================================================
 Name        : AxisPos.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : Calculates movement steps
 ===========================================================================
 */

#include "Common.h"

#ifndef _AxisPos_H
#define _AxisPos_H
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
	// Uses 6 bits to update each of the 6 axis for channel 1
	uint8_t printC1 = 0;

	// Angle values for channel 2
	uint16_t a1c2 = 0;
	uint16_t a2c2 = 0;
	uint16_t a3c2 = 0;
	uint16_t a4c2 = 0;
	uint16_t a5c2 = 0;
	uint16_t a6c2 = 0;
	// Uses 6 bits to update each of the 6 axis for channel 2
	uint8_t printC2 = 0;
	
public:
	void drawAxisPos(UTFT);
	void drawAxisPosUpdate(UTFT);
	void drawAxisPosUpdateM(UTFT, uint16_t, bool);
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
#endif // _AxisPos_H