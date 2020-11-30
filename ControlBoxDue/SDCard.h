// SDCard.h

#ifndef _SDCard_h
#define _SDCard_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
class CANBus;

class SDCard
{
 protected:


 public:
	bool startSD();
	void writeFile(uint8_t, int);
	void writeFile(String);
	void writeFile(int);
	void writeFileln();
	String readFile();
};

#endif

