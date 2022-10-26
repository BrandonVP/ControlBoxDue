// CANBus.h
#include <due_can.h>
#include "variant.h"
#include "Common.h"

#ifndef _CANBus_h
#define _CANBus_h
class AxisPos;

class CANBus
{
 public:
	 CAN_FRAME incoming;
	 CAN_FRAME outgoing;

 public:
	 void sendFrame(uint16_t, byte*);
	 void processFrame(AxisPos&, UTFT&);
	 void startCAN();

};
#endif
