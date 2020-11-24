// 
// 
// 

#include "CANBus.h"
#include <due_can.h>
#include "variant.h"
void CANBus::init()
{


}

CANBus::CANBus()
{


}

void CANBus::startCAN()
{
    // Initialize CAN0 and CAN1, Set the proper baud rates here
    Can0.begin(CAN_BPS_500K);
    Can0.watchFor();
}

void CANBus::sendData(byte *frame, int id)
{
    CAN_FRAME myFrame;
    myFrame.id = id;
    myFrame.length = 8;
    byte test2[8] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22 };

    myFrame.data.byte[0] = frame[0];
    myFrame.data.byte[1] = frame[1];
    myFrame.data.byte[2] = frame[2];
    myFrame.data.byte[3] = frame[3];
    myFrame.data.byte[4] = frame[4];
    myFrame.data.byte[5] = frame[5];
    myFrame.data.byte[6] = frame[6];
    myFrame.data.byte[7] = frame[7];

    Can0.sendFrame(myFrame);
    return;
}

void CANBus::receiveCAN()
{
    CAN_FRAME incoming;

    if (Can0.available() > 0) {
        Can0.read(incoming);
    }
    if (Can1.available() > 0) {
        Can1.read(incoming);
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