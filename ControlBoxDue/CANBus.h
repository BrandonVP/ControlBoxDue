/*
 ===========================================================================
 Name        : CANBus.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : CANBus manages the CAN bus hardware
 ===========================================================================
 */

#include <due_can.h>
#include "variant.h"
#include "Common.h"

#ifndef _CANBus_H
#define _CANBus_H
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
#endif // _CANBus_H