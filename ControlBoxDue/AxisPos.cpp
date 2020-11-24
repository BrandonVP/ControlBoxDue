// 
// 
// 
#include <due_can.h>
#include "variant.h"
#include "AxisPos.h"

AxisPos::AxisPos()
{
	
}


void AxisPos::init()
{
	
}

void armSearch()
{
	bool isDoneCh1 = false;
	bool isDoneCh2 = false;

	unsigned long timer = millis();
	while(isDoneCh1 || (millis() - timer > 10000))
	if ((millis() - timer) > 1000)
	{

	}

	timer = millis();
	while (isDoneCh2 || (millis() - timer > 0000))
		if ((millis() - timer) > 1000)
		{

		}
}
void AxisPos::drawAxisPos(UTFT LCD)
{
	LCD.setColor(0xFFFF); // text color
	LCD.setBackColor(0xC618); // text background
	LCD.print("180", 205, 48);
	LCD.print("180", 205, 93);
	LCD.print("90", 205, 138);
	LCD.print("180", 205, 183);
	LCD.print("180", 205, 228);
	LCD.print("180", 205, 273);
}
void AxisPos::updateAxisPos(UTFT LCD)
{
	LCD.setColor(0xFFFF); // text color
	LCD.setBackColor(0xC618); // text background
	LCD.print("180", 205, 48);
	LCD.print("180", 205, 93);
	LCD.print("90", 205, 138);
	LCD.print("180", 205, 183);
	LCD.print("180", 205, 228);
	LCD.print("180", 205, 273);
}
void requestAxisPos()
{

}


