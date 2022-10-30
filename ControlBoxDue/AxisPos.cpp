// 
// 
// 
#include <due_can.h>
#include "AxisPos.h"
#include "definitions.h"

#define ARM1_POSITION   0x1A0
#define ARM2_POSITION   0x2A0
void AxisPos::updateAxisPos(CAN_FRAME axisFrame)
{
	// Need these temp values to calculate "CRC"
	uint8_t crc, grip;
	uint16_t a1, a2, a3, a4, a5, a6;

	//|                          data                              |
	//|  0  |  1   |   2   |   3   |   4   |   5   |   6   |   7   |
	//| 0-7 | 8-15 | 16-23 | 24-31 | 32-39 | 40-47 | 48-55 | 56-63 |

	//|  crc  |  grip |   a6   |   a5   |   a4   |   a3   |   a2   |   a1   |
	//|  0-4  |  5-9  |  10-18 |  19-27 |  28-36 |  37-45 |  46-54 |  55-63 |
	a1 = ((axisFrame.data.byte[6] & 0x01) << 8) | axisFrame.data.byte[7];
	a2 = (((axisFrame.data.byte[5] & 0x03)) << 7) | (axisFrame.data.byte[6] >> 1);
	a3 = (((axisFrame.data.byte[4] & 0x07)) << 6) | (axisFrame.data.byte[5] >> 2);
	a4 = (((axisFrame.data.byte[3] & 0x0F)) << 5) | (axisFrame.data.byte[4] >> 3);
	a5 = (((axisFrame.data.byte[2] & 0x1F)) << 4) | (axisFrame.data.byte[3] >> 4);
	a6 = (((axisFrame.data.byte[1] & 0x3F)) << 3) | (axisFrame.data.byte[2] >> 5);
	grip = ((axisFrame.data.byte[0] & 0x7) << 2)  | (axisFrame.data.byte[1] >> 6);
	crc = ((axisFrame.data.byte[0]) >> 3);

	// A very simple "CRC"
	if (crc == generateBitCRC(a1, a2, a3, a4, a5, a6, grip))
	{
		// Determine which channel to write values too
		if (axisFrame.id == ARM1_POSITION)
		{
			a1c1 = a1;
			a2c1 = a2;
			a3c1 = a3;
			a4c1 = a4;
			a5c1 = a5;
			a6c1 = a6;
		}
		else if (axisFrame.id == ARM2_POSITION)
		{
			a1c2 = a1;
			a2c2 = a2;
			a3c2 = a3;
			a4c2 = a4;
			a5c2 = a5;
			a6c2 = a6;
		}
	}
}

// Update and draw the Axis positions on the view page
void AxisPos::drawAxisPos(UTFT LCD)
{
	// Text color
	LCD.setColor(0xFFFF);

	// Text background color
	LCD.setBackColor(0xC618);

	// Draw angles 
	LCD.printNumI(a1c1, 195, 48, 3, '0');
	LCD.printNumI(a2c1, 195, 93, 3, '0');
	LCD.printNumI(a3c1, 195, 138, 3, '0');
	LCD.printNumI(a4c1, 195, 183, 3, '0');
	LCD.printNumI(a5c1, 195, 228, 3, '0');
	LCD.printNumI(a6c1, 195, 273, 3, '0');
	LCD.printNumI(a1c2, 305, 48, 3, '0');
	LCD.printNumI(a2c2, 305, 93, 3, '0');
	LCD.printNumI(a3c2, 305, 138, 3, '0');
	LCD.printNumI(a4c2, 305, 183, 3, '0');
	LCD.printNumI(a5c2, 305, 228, 3, '0');
	LCD.printNumI(a6c2, 305, 273, 3, '0');
}

// Get angle for programming
int AxisPos::getA1C1()
{
	return a1c1;
}
int AxisPos::getA2C1()
{
	return a2c1;
}
int AxisPos::getA3C1()
{
	return a3c1;
}
int AxisPos::getA4C1()
{
	return a4c1;
}
int AxisPos::getA5C1()
{
	return a5c1;
}
int AxisPos::getA6C1()
{
	return a6c1;
}
int AxisPos::getA1C2()
{
	return a1c2;
}
int AxisPos::getA2C2()
{
	return a2c2;
}
int AxisPos::getA3C2()
{
	return a3c2;
}
int AxisPos::getA4C2()
{
	return a4c2;
}
int AxisPos::getA5C2()
{
	return a5c2;
}
int AxisPos::getA6C2()
{
	return a6c2;
}
