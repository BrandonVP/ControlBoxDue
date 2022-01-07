/*
 Name:		CANBusWifi.h
 Created:	10/22/2021 12:40:14 PM
 Author:	Brandon Van Pelt
 Editor:	http://www.visualmicro.com
*/

#ifndef _CANBusWifi_h
#define _CANBusWifi_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#define ARRAY_SIZE 8

// States
#define START_BYTE              (0)
#define PACKET_LENGTH           (1)
#define CAN_BUS_ID              (2)
#define CAN_BUS_DATA            (3)
#define END_BYTE                (4)
// 
#define STARTING_BYTE           (0xFE)
#define ENDING_BYTE             (0xFD)
#define PACKET_SIZE             (0x09)

struct CAN_Message
{
	volatile uint16_t id;
	volatile uint8_t data[8];
};

class CANBusWifi
{
private:
	uint8_t state = 0;
	uint8_t packetIndex = 0;
	struct CAN_Message CAN_FRAME1;
public:
	bool readFrame(CAN_Message&);
	void sendFrame(CAN_Message);
};

#endif

