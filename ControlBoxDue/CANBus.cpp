/*
 ===========================================================================
 Name        : CANBus.cpp
 Author      : Brandon Van Pelt
 Created	 :
 Description : CANBus manages the CAN bus hardware
 ===========================================================================
 */

#include "CANBus.h"
#include "variant.h"
#include "definitions.h"

 /*=========================================================
 CRC - https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code
 ===========================================================*/
#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */
#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))

// Create CRC table
void CANBus::initCRC(void)
{
	uint8_t  remainder;

	// Compute the remainder of each possible dividend
	for (int dividend = 0; dividend < 256; ++dividend)
	{
		//Start with the dividend followed by zeros
		remainder = dividend << (WIDTH - 8);

		// Perform modulo-2 division, a bit at a time
		for (uint8_t bit = 8; bit > 0; --bit)
		{
			// Try to divide the current data bit.
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = (remainder << 1);
			}
		}

		// Store the result into the table.
		crcTable[dividend] = remainder;
	}
}

// All sent messages have a CRC added to data[7]
uint8_t CANBus::generateCRC(uint8_t const message[], int nBytes)
{
	uint8_t data;
	uint8_t remainder = 0;

	// Divide the message by the polynomial, a byte at a time.
	for (int byte = 0; byte < nBytes; ++byte)
	{
		data = message[byte] ^ (remainder >> (WIDTH - 8));
		remainder = crcTable[data] ^ (remainder << 8);
	}

	// The final remainder is the CRC.
	return (remainder);
}

// Initialize CAN1 and set the baud rate
void CANBus::startCAN()
{
    Can0.begin(CAN_BPS_500K);
    Can0.watchFor();
}

// Adds CRC and sends out CAN Bus message
void CANBus::sendFrame(uint16_t id, byte* frame)
{
    outgoing.extended = false;
    outgoing.id = id;
    outgoing.length = 8;
    frame[7] = generateCRC(frame, 7);
    memcpy(outgoing.data.byte, frame, 8);
    Can0.sendFrame(outgoing);
}

// Process incoming CAN Bus messages
void CANBus::processFrame(CANBus can1, AxisPos& axisPos)
{
    // If inbox buffer has a message
    if (Can0.available() > 0)
    {
        Can0.read(incoming);

        switch (incoming.id)
        {
        case ARM1_POSITION: // Arm 1 axis positions
            axisPos.updateAxisPos(can1, incoming);
            break;
        case ARM2_POSITION: //  Arm 2 axis positions
            axisPos.updateAxisPos(can1, incoming);
            break;
        case CONTROL1_RX: // C1 Confirmation
            Arm1Ready = true;
            break;
        case CONTROL2_RX: // C2 Confirmation
            Arm2Ready = true;
            break;
        }
    }
}