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

    // Outgoing message ID
    myFrame.id = id;

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

bool CANBus::hasMSGr() {
    bool temp = hasMSG;
    hasMSG = true;
    return temp;
}

// Method used to manually get the ID and byte array
void CANBus::getMessage(test& a, int& b)
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

//MOVE TO METHODS
/*
CAN_FRAME incoming;
static unsigned long lastTime = 0;

if (Can0.available() > 0) {
    Can0.read(incoming);
    test = incoming.data.byte[0];
    sendData(test);


    SD.begin(CSPIN);        //SD Card is initialized
    //SD.remove("Test2.txt"); //remove any existing file with this name
    myFile = SD.open("Test2.txt", FILE_WRITE);  //file created and opened for writing

    if (myFile)        //file has really been opened
    {
        myFile.print("ID: ");
        myFile.print(incoming.id);
        myFile.print(" MSG:");
        for (int count = 0; count < incoming.length; count++) {
            myFile.print(incoming.data.bytes[count], HEX);
            myFile.print(" ");
        }
        myFile.println();
        myFile.close();

    }
}
*/