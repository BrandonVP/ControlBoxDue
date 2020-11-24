// CANBus.h

#ifndef _CANBus_h
#define _CANBus_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class CANBus
{
 private:


 public:
	CANBus();
	void init();
	void receiveCAN();
	void sendData(byte*, int);
	void startCAN();
};

//extern CANBus ;

#endif

