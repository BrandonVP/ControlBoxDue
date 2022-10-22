// 
// 
// 
#include <due_can.h>
#include "variant.h"
#include "AxisPos.h"

//
void AxisPos::sendRequest(CANBus can1)
{
	uint16_t channel1[2] = { ARM1ID, ARM1RXID };
	uint16_t channel2[2] = { ARM2ID, ARM2RXID };
	uint8_t requestAngles[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	requestAngles[1] = LOWER;
	can1.sendFrame(ARM1ID, requestAngles);
	can1.sendFrame(ARM2ID, requestAngles);
	requestAngles[1] = UPPER;
	can1.sendFrame(ARM1ID, requestAngles);
	can1.sendFrame(ARM2ID, requestAngles);
}

//
void AxisPos::updateAxisPos(CANBus can1, uint8_t channel)
{
	SerialUSB.println("updateAxisPos");

	// Request CAN frame addressed to paremeter ID
	uint8_t* data = can1.getFrame();

	// Need these temp values to calculate "CRC"
	uint8_t crc, grip;
	uint16_t a1, a2, a3, a4, a5, a6;

	//|                          data                              |
	//|  0  |  1   |   2   |   3   |   4   |   5   |   6   |   7   |
	//| 0-7 | 8-15 | 16-23 | 24-31 | 32-39 | 40-47 | 48-55 | 56-63 |

	//|  crc  |  grip |   a6   |   a5   |   a4   |   a3   |   a2   |   a1   |
	//|  0-4  |  5-9  |  10-18 |  19-27 |  28-36 |  37-45 |  46-54 |  55-63 |
	a1 = ((data[6] & 0x01) << 8)   | data[7];
	a2 = (((data[5] & 0x03)) << 7) | (data[6] >> 1);
	a3 = (((data[4] & 0x07)) << 6) | (data[5] >> 2);
	a4 = (((data[3] & 0x0F)) << 5) | (data[4] >> 3);
	a5 = (((data[2] & 0x1F)) << 4) | (data[3] >> 4);
	a6 = (((data[1] & 0x3F)) << 3) | (data[2] >> 5);
	grip = ((data[0] & 0x7) << 2)  | (data[1] >> 6);
	crc = ((data[0]) >> 3);

	// A very simple "CRC"
	//if (crc == (a1 % 2) + (a2 % 2) + (a3 % 2) + (a4 % 2) + (a5 % 2) + (a6 % 2) + (grip % 2) + 1)
	if (true)
	{
		// Determine which channel to write values too
		if (channel == ARM1_POSITION)
		{
			a1c1 = a1;
			a2c1 = a2;
			a3c1 = a3;
			a4c1 = a4;
			a5c1 = a5;
			a6c1 = a6;
		}
		else //if (channel == POSITION_ID_2)
		{
			SerialUSB.println("in");
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
