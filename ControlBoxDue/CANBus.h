// CANBus.h
#include "SDCard.h"
#include <due_can.h>
#include "variant.h"

#ifndef _CANBus_h
#define _CANBus_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
class Program;

class CANBus
{
 public:
	 CAN_FRAME incoming;
	 CAN_FRAME outgoing;
	 uint8_t MSGFrame[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	 typedef byte frame[8];

 public:
	 void sendFrame(uint16_t, byte*);
	 bool msgCheck(uint16_t, uint8_t, int8_t);
	 uint8_t* getFrame();
	 void resetMSGFrame();
	 uint8_t processFrame();
	 void startCAN();
};
#endif
