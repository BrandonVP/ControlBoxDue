/*
 ===========================================================================
 Name        : AxisPos.cpp
 Author      : Brandon Van Pelt
 Created	 : 
 Description : Calculates movement steps
 ===========================================================================
 */

#include <due_can.h>
#include "AxisPos.h"
#include "definitions.h"

 // Checks a single bit of binary number
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

// Decodes incoming message and updates the current axis position in degrees
void AxisPos::updateAxisPos(CANBus can1, CAN_FRAME axisFrame)
{
	// Need these temp values to calculate "CRC"
	uint8_t crc, grip;
	uint16_t a1, a2, a3, a4, a5, a6;

	//|                          data                              |
	//|  0  |  1   |   2   |   3   |   4   |   5   |   6   |   7   |
	//| 0-7 | 8-15 | 16-23 | 24-31 | 32-39 | 40-47 | 48-55 | 56-63 |

	//|  crc  |  grip |   a6   |   a5   |   a4   |   a3   |   a2   |   a1   |
	//|  0-4  |  5-9  |  10-18 |  19-27 |  28-36 |  37-45 |  46-54 |  55-63 |
	a1 = ((axisFrame.data.byte[5] & 0x01) << 8) | axisFrame.data.byte[6];
	a2 = (((axisFrame.data.byte[4] & 0x03)) << 7) | (axisFrame.data.byte[5] >> 1);
	a3 = (((axisFrame.data.byte[3] & 0x07)) << 6) | (axisFrame.data.byte[4] >> 2);
	a4 = (((axisFrame.data.byte[2] & 0x0F)) << 5) | (axisFrame.data.byte[3] >> 3);
	a5 = (((axisFrame.data.byte[1] & 0x1F)) << 4) | (axisFrame.data.byte[2] >> 4);
	a6 = (((axisFrame.data.byte[0] & 0x3F)) << 3) | (axisFrame.data.byte[1] >> 5);
	grip = (axisFrame.data.byte[0] >> 6);
	crc = (axisFrame.data.byte[7]);

	if (crc == can1.generateCRC(axisFrame.data.byte, 7))
	{
		// Determine which channel to write values too
		if (axisFrame.id == ARM1_POSITION)
		{
			if (a1c1 != a1){printC1 |= (1 << 0);}
			if (a2c1 != a2){printC1 |= (1 << 1);}
			if (a3c1 != a3){printC1 |= (1 << 2);}
			if (a4c1 != a4){printC1 |= (1 << 3);}
			if (a5c1 != a5){printC1 |= (1 << 4);}
			if (a6c1 != a6){printC1 |= (1 << 5);}
			a1c1 = a1;
			a2c1 = a2;
			a3c1 = a3;
			a4c1 = a4;
			a5c1 = a5;
			a6c1 = a6;
		}
		else if (axisFrame.id == ARM2_POSITION)
		{
			if (a1c2 != a1) { printC2 |= (1 << 0); }
			if (a2c2 != a2) { printC2 |= (1 << 1); }
			if (a3c2 != a3) { printC2 |= (1 << 2); }
			if (a4c2 != a4) { printC2 |= (1 << 3); }
			if (a5c2 != a5) { printC2 |= (1 << 4); }
			if (a6c2 != a6) { printC2 |= (1 << 5); }
			a1c2 = a1;
			a2c2 = a2;
			a3c2 = a3;
			a4c2 = a4;
			a5c2 = a5;
			a6c2 = a6;
		}
		/*
		Serial.print("updateAxisPos: ");
		Serial.print(CHECK_BIT(printC1, 5));
		Serial.print(" ");
		Serial.print(CHECK_BIT(printC1, 4));
		Serial.print(" ");
		Serial.print(CHECK_BIT(printC1, 3));
		Serial.print(" ");
		Serial.print(CHECK_BIT(printC1, 2));
		Serial.print(" ");
		Serial.print(CHECK_BIT(printC1, 1));
		Serial.print(" ");
		Serial.println(CHECK_BIT(printC1, 0));
		*/
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

// Update and draw the Axis positions on the view page
void AxisPos::drawAxisPosUpdate(UTFT LCD)
{
	// Text color
	LCD.setColor(0xFFFF);

	// Text background color
	LCD.setBackColor(0xC618);

	// Draw angles 
	if (CHECK_BIT(printC1, 0))
	{
		LCD.printNumI(a1c1, 195, 48, 3, '0');
		printC1 &= ~(1 << 0);
	}
	if (CHECK_BIT(printC1, 1))
	{
		LCD.printNumI(a2c1, 195, 93, 3, '0');
		printC1 &= ~(1 << 1);
	}
	if (CHECK_BIT(printC1, 2))
	{
		LCD.printNumI(a3c1, 195, 138, 3, '0');
		printC1 &= ~(1 << 2);
	}
	if (CHECK_BIT(printC1, 3))
	{
		LCD.printNumI(a4c1, 195, 183, 3, '0');
		printC1 &= ~(1 << 3);
	}
	if (CHECK_BIT(printC1, 4))
	{
		LCD.printNumI(a5c1, 195, 228, 3, '0');
		printC1 &= ~(1 << 4);
	}
	if (CHECK_BIT(printC1, 5))
	{
		LCD.printNumI(a6c1, 195, 273, 3, '0');
		printC1 &= ~(1 << 5);
	}

	if (CHECK_BIT(printC2, 0))
	{
		LCD.printNumI(a1c2, 305, 48, 3, '0');
		printC2 &= ~(1 << 0);
	}
	if (CHECK_BIT(printC2, 1))
	{
		LCD.printNumI(a2c2, 305, 93, 3, '0');
		printC2 &= ~(1 << 1);
	}
	if (CHECK_BIT(printC2, 2))
	{
		LCD.printNumI(a3c2, 305, 138, 3, '0');
		printC2 &= ~(1 << 2);
	}
	if (CHECK_BIT(printC2, 3))
	{
		LCD.printNumI(a4c2, 305, 183, 3, '0');
		printC2 &= ~(1 << 3);
	}
	if (CHECK_BIT(printC2, 4))
	{
		LCD.printNumI(a5c2, 305, 228, 3, '0');
		printC2 &= ~(1 << 4);
	}
	if (CHECK_BIT(printC2, 5))
	{
		LCD.printNumI(a6c2, 305, 273, 3, '0');
		printC2 &= ~(1 << 5);
	}
}

// Update and draw the Axis positions on the view page
void AxisPos::drawAxisPosUpdateM(UTFT LCD, uint16_t armID, bool print)
{
	// Text color
	LCD.setColor(0xFFFF);

	// Text background color
	LCD.setBackColor(menuBtnColor);

	// Draw angles 
	if (armID == ARM1_MANUAL)
	{
		if (CHECK_BIT(printC1, 0) || print)
		{
			LCD.printNumI(a1c1, 134, 142, 3, '0');
			printC1 &= ~(1 << 0);
		}
		if (CHECK_BIT(printC1, 1) || print)
		{
			LCD.printNumI(a2c1, 191, 142, 3, '0');
			printC1 &= ~(1 << 1);
		}
		if (CHECK_BIT(printC1, 2) || print)
		{
			LCD.printNumI(a3c1, 249, 142, 3, '0');
			printC1 &= ~(1 << 2);
		}
		if (CHECK_BIT(printC1, 3) || print)
		{
			LCD.printNumI(a4c1, 307, 142, 3, '0');
			printC1 &= ~(1 << 3);
		}
		if (CHECK_BIT(printC1, 4) || print)
		{
			LCD.printNumI(a5c1, 365, 142, 3, '0');
			printC1 &= ~(1 << 4);
		}
		if (CHECK_BIT(printC1, 5) || print)
		{
			LCD.printNumI(a6c1, 423, 142, 3, '0');
			printC1 &= ~(1 << 5);
		}
	}
	else if (armID == ARM2_MANUAL)
	{
		if (CHECK_BIT(printC2, 0) || print)
		{
			LCD.printNumI(a1c2, 134, 142, 3, '0');
			printC2 &= ~(1 << 0);
		}
		if (CHECK_BIT(printC2, 1) || print)
		{
			LCD.printNumI(a2c2, 191, 142, 3, '0');
			printC2 &= ~(1 << 1);
		}
		if (CHECK_BIT(printC2, 2) || print)
		{
			LCD.printNumI(a3c2, 249, 142, 3, '0');
			printC2 &= ~(1 << 2);
		}
		if (CHECK_BIT(printC2, 3) || print)
		{
			LCD.printNumI(a4c2, 307, 142, 3, '0');
			printC2 &= ~(1 << 3);
		}
		if (CHECK_BIT(printC2, 4) || print)
		{
			LCD.printNumI(a5c2, 365, 142, 3, '0');
			printC2 &= ~(1 << 4);
		}
		if (CHECK_BIT(printC2, 5) || print)
		{
			LCD.printNumI(a6c2, 423, 142, 3, '0');
			printC2 &= ~(1 << 5);
		}
	}
}

void AxisPos::setPrintC1(uint8_t newValue)
{
	printC1 = newValue;
}

void AxisPos::setPrintC2(uint8_t newValue)
{
	printC2 = newValue;
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
