// CANBus manages the CAN bus hardware
#include "CANBus.h"
#include "definitions.h"

void CANBus::startCAN()
{
    // Initialize CAN1 and set the proper baud rates here
    Can0.begin(CAN_BPS_500K);
    Can0.watchFor();
}

// Send a CAN Bus message
void CANBus::sendFrame(uint16_t id, byte* frame)
{
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