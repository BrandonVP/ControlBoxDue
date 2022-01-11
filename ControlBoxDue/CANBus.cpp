// CANBus manages the CAN bus hardware

#include "CANBus.h"
#include "CANBusWiFi.h"

void CANBus::startCAN()
{
    // Initialize CAN1 and set the proper baud rates here
    Can0.begin(CAN_BPS_500K);
    Can0.watchFor();
    return;
}

// CAN Bus send message
void CANBus::sendFrame(uint16_t id, byte* frame)
{
    Serial3.write(0xFE);
    Serial3.write(0x09);
    Serial3.write(id);
    for (uint8_t i = 0; i < ARRAY_SIZE; i++)
    {
        Serial3.write(frame[i]);
    }
    Serial3.write(0xFD);

    // Disable extended frames
    outgoing.extended = false;

    // Outgoing message ID
    outgoing.id = id;

    // Message length
    outgoing.length = 8;

    // Assign object to message array
    outgoing.data.byte[0] = frame[0];
    outgoing.data.byte[1] = frame[1];
    outgoing.data.byte[2] = frame[2];
    outgoing.data.byte[3] = frame[3];
    outgoing.data.byte[4] = frame[4];
    outgoing.data.byte[5] = frame[5];
    outgoing.data.byte[6] = frame[6];
    outgoing.data.byte[7] = frame[7];

    // Send object out
    Can0.sendFrame(outgoing);

    return;
}

// Get and return message frame from specified rxID
uint8_t* CANBus::getFrame()
{
    return MSGFrame;
}

// Resets objects uint8_t array back to zero
void CANBus::resetMSGFrame()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        MSGFrame[i] = 0x00;
    }
}

// Check if value exists in incoming message, used for confirmation
bool CANBus::msgCheck(uint16_t ID, uint8_t value, int8_t pos)
{
    // If buffer inbox has a message
    if (Can0.available() > 0)
    {
        Can0.read(incoming);
        if (incoming.id == ID && incoming.data.byte[pos] == value)
        {
            return true;
        }  
    }
    return false;
}

//
uint8_t CANBus::processFrame()
{
    // If buffer inbox has a message
    if (Can0.available() > 0)
    {
        Can0.read(incoming);
        Serial.print(F("ID: "));
        Serial.println(incoming.id, HEX);
        for (uint8_t i = 0; i < 8; i++)
        {
            MSGFrame[i] = incoming.data.byte[i];
        }
        if (incoming.id == 0xC1)
        {
            Serial.print(F("Value: "));
            Serial.println((incoming.data.byte[1]));
            return incoming.data.byte[1];
        }
        if (incoming.id == 0xC2)
        {
            Serial.print(F("Value: "));
            Serial.println((incoming.data.byte[1] + 3));
            return incoming.data.byte[1] + 3;
        }
    }
    return 0;
}
