// CANBus manages the CAN bus hardware
#include "CANBus.h"

void CANBus::startCAN()
{
    // Initialize CAN1 and set the proper baud rates here
    Can0.begin(CAN_BPS_500K);
    Can0.watchFor();
}

// Send a CAN Bus message
void CANBus::sendFrame(uint16_t id, byte* frame)
{
    /*
    // TODO: What is this? Attempt to use WiFi?
    Serial3.write(0xFE);
    Serial3.write(0x09);
    Serial3.write(id);
    for (uint8_t i = 0; i < ARRAY_SIZE; i++)
    {
        Serial3.write(frame[i]);
    }
    Serial3.write(0xFD);
    */
    
    outgoing.extended = false;
    outgoing.id = id;
    outgoing.length = 8;
    memcpy(outgoing.data.byte, frame, 8);
    Can0.sendFrame(outgoing);
}

void CANBus::processFrame(AxisPos& axisPos, UTFT& myGLCD)
{
    // If buffer inbox has a message
    if (Can0.available() > 0)
    {
        Can0.read(incoming);

        switch (incoming.id)
        {
        case ARM1_POSITION: // Arm 1 axis positions
            axisPos.updateAxisPos(incoming);
            if (page == 1)
            {
                axisPos.drawAxisPos(myGLCD);
            }
            break;
        case ARM2_POSITION: //  Arm 2 axis positions
            axisPos.updateAxisPos(incoming);
            if (page == 1)
            {
                axisPos.drawAxisPos(myGLCD);
            }
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
