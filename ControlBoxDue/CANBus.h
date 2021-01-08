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
class Program;

class CANBus
{
 public:
	uint8_t MSGFrame[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	bool hasMSG;

 private:
	 SDCard SDPrint;
	 typedef byte frame[8];

 public:
	CANBus();
	void getMessage(frame&, int&);
	void startCAN();
	void sendFrame(uint16_t, byte*);
	bool msgCheck(uint16_t, uint8_t, int8_t);
	uint8_t* getFrame(uint16_t);
	uint16_t getFrameID();
	bool hasMSGr();
	void resetMSGFrame();
};
#endif
