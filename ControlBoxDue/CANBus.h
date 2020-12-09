// CANBus.h
#include "SDCard.h"

#ifndef _CANBus_h
#define _CANBus_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
class SDCard;

class CANBus
{
 private:
	 SDCard SDPrint;
	 typedef byte test[8];
 public:

	CANBus();
	void getMessage(test&, int&);
	void startCAN();
	void sendFrame(uint16_t, byte*);
	bool getFrame(uint16_t);
};
#endif

