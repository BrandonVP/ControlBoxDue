// CANBus manages the CAN bus hardware

#include "CANBus.h"
#include <due_can.h>
#include "variant.h"

CANBus::CANBus()
{

}

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
    // Create message object
    CAN_FRAME myFrame;
    myFrame.extended = false;
    // Outgoing message ID
    myFrame.id = id;
    //Serial.print("ID: ");
    //Serial.println(id);
    // Message length
    myFrame.length = 8;

    // Assign object to message array
    myFrame.data.byte[0] = frame[0];
    myFrame.data.byte[1] = frame[1];
    myFrame.data.byte[2] = frame[2];
    myFrame.data.byte[3] = frame[3];
    myFrame.data.byte[4] = frame[4];
    myFrame.data.byte[5] = frame[5];
    myFrame.data.byte[6] = frame[6];
    myFrame.data.byte[7] = frame[7];
    /*
    Serial.print("MSG: ");
    Serial.print(frame[0]);
    Serial.print(" ");
    Serial.print(frame[1]);
    Serial.print(" ");
    Serial.print(frame[2]);
    Serial.print(" ");
    Serial.print(frame[3]);
    Serial.print(" ");
    Serial.print(frame[4]);
    Serial.print(" ");
    Serial.print(frame[5]);
    Serial.print(" ");
    Serial.print(frame[6]);
    Serial.print(" ");
    Serial.println(frame[7]);
    */
    // Send object out
    Can0.sendFrame(myFrame);
    return;
}

// Method used for CAN recording
uint8_t* CANBus::getFrame(uint16_t IDFilter)
{
    // Create object to save message
    CAN_FRAME incoming;

    // If buffer inbox has a message
    if (Can0.available() > 0) 
    {
        Can0.read(incoming);
        for (int i = 0; i < 8; i++)
        {
            MSGFrame[i] = incoming.data.byte[i];
        }
        hasMSG = false;
    }
    return MSGFrame;
}

void CANBus::resetMSGFrame()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        MSGFrame[i] = 0x00;
    }
}

// Method used for CAN recording
bool CANBus::msgCheck(uint16_t ID, uint8_t value, int8_t pos)
{
    // Create object to save message
    CAN_FRAME incoming;



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

uint16_t CANBus::getFrameID()
{
    // Create object to save message
    CAN_FRAME incoming;

    // If buffer inbox has a message
    if (Can0.available() > 0)
    {
        Can0.read(incoming);
    }
    return incoming.id;
}

// Reset hasMSG to true
bool CANBus::hasMSGr() {
    bool temp = hasMSG;
    hasMSG = true;
    return temp;
}

// Method used to manually get the ID and byte array
void CANBus::getMessage(frame& a, int& b)
{
    CAN_FRAME incoming;
    if (Can0.available() > 0) {
        Can0.read(incoming);
        b = incoming.id;

        for (int count = 0; count < incoming.length; count++) {
            a[count] = incoming.data.bytes[count];
        }

    }
}
