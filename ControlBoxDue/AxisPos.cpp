// 
// 
// 
#include <due_can.h>
#include "variant.h"
#include "AxisPos.h"



// Default Constructor
AxisPos::AxisPos()
{
	
}

void AxisPos::armSearch()
{
	bool isDone = true;
	uint8_t requestAngles[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	
	
	
	requestAngles[1] = 0x01;
	can1.sendFrame(ARM1ID, requestAngles);
	delay(50);
	unsigned long timer = millis();
	while (isDone && (millis() - timer < 1000))
	{
		uint8_t* temp = can1.getFrame(0x0C1);
		if (temp[0] == 0x01)
		{
			isDone = can1.hasMSGr();
			a1c1 = temp[2] + temp[3];
			a2c1 = temp[4] + temp[5];
			a3c1 = temp[6] + temp[7];
		}
	}
	
	can1.hasMSGr();
	isDone = true;
	
	requestAngles[1] = 0x02;
	can1.sendFrame(ARM1ID, requestAngles);
	delay(50);
	timer = millis();
	while (isDone && (millis() - timer < 1000))
	{
		uint8_t* temp =	can1.getFrame(0x0C1);
		if (temp[0] == 0x02)
		{
			isDone = can1.hasMSGr();
			a4c1 = temp[2] + temp[3];
			a5c1 = temp[4] + temp[5];
			a6c1 = temp[6] + temp[7];
		}
	}
	/*
	isDone = true;
	timer = millis();
	requestAngles[1] = 0x01;
	can1.sendFrame(ARM2, requestAngles);
	while (isDone || (millis() - timer < 1000))
	{
		uint8_t* temp = can1.getFrame(0x0C2);
		armch1 a1c1 = temp[2] + temp[3];
		armch1 a2c1 = temp[4] + temp[5];
		armch1 a3c1 = temp[5] + temp[7];
		isDone = false;
	}
	isDone = true;
	timer = millis();
	requestAngles[1] = 0x02;
	can1.sendFrame(ARM2, requestAngles);
	while (isDone || (millis() - timer < 1000))
	{
		uint8_t* temp = can1.getFrame(0x0C2);
		armch1 a4c1 = temp[2] + temp[3];
		armch1 a5c1 = temp[4] + temp[5];
		armch1 a6c1 = temp[6] + temp[7];
		isDone = false;
	}
	*/
}

void AxisPos::drawAxisPos(UTFT LCD)
{
	armSearch();
	LCD.setColor(0xFFFF); // text color
	LCD.setBackColor(0xC618); // text background
	if (a1c1 >= 0)
	{
		LCD.printNumI(a1c1, 205, 48);
		LCD.printNumI(a2c1, 205, 93);
		LCD.printNumI(a3c1, 205, 138);
		LCD.printNumI(a4c1, 205, 183);
		LCD.printNumI(a5c1, 205, 228);
		LCD.printNumI(a6c1, 205, 273);
	}
}

void AxisPos::updateAxisPos()
{
	armSearch();
}

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


