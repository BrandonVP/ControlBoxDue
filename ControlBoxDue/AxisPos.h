// AxisPos.h
#include <UTFT.h>
#ifndef _AxisPos_h
#define _AxisPos_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
typedef uint8_t angle[2][8];

class AxisPos
{
 private:
	 uint8_t numOfArms;
	 angle arm1 = {
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 };
	 angle arm2 = {
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 };
	 angle arm3 = {
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 };
	 angle arm4 = {
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 };

	 UTFT LCD;

 public:
	void init();
	AxisPos();
	void drawAxisPos(UTFT);
	void updateAxisPos(UTFT);


};


#endif

