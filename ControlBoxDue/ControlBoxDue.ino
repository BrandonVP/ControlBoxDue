/*
 Name:    ControlBoxDue.ino
 Created: 11/15/2020 8:27:18 AM
 Author:  Brandon Van Pelt
*/

#include <LinkedList.h>
#include <UTouchCD.h>
#include <memorysaver.h>
#include <SPI.h>
#include <UTFT.h>
#include <UTouch.h>
#include "AxisPos.h"
#include "CANBus.h"
#include "SDCard.h"
#include "Program.h"
#include "definitions.h"
#include "icons.h"

// Initialize display
//(byte model, int RS, int WR, int CS, int RST, int SER)
UTFT myGLCD(ILI9488_16, 7, 38, 9, 10);    
//RTP: byte tclk, byte tcs, byte din, byte dout, byte irq
UTouch  myTouch(2, 6, 3, 4, 5);      

// For touch controls
int x, y;

// External import for fonts
extern uint8_t SmallFont[];
extern uint8_t BigFont[];

// Objects for keeping track of current angle positions for both arms
AxisPos arm1;
AxisPos arm2;

// Object to control CAN Bus hardware
CANBus can1;

// Object to control SD Card Hardware
SDCard sdCard;

// AxisPos object used to get current angles of robot arm
AxisPos axisPos;

// Linked list of nodes for a program
LinkedList<Program*> runList = LinkedList<Program*>();

// Current selected program
uint8_t selectedProgram = 0;

// Current selected node
uint8_t selectedNode = 0;

// Has a program been loaded
bool programLoaded = false;

// UPDATE THESE TWO LINES FROM SD CARD DURING SETUP
// List for populated program slots
uint8_t emptyList[10] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
// This array should populate from SD CARD
String aList[10] = { "Program1", "Program2", "Program3", "Program4", "Program5", "Program6", "Program7", "Program8", "Program9", "Program10" };

uint8_t controlPage = 1;
bool programOpen = false;

/*=========================================================
    Framework Functions
===========================================================*/
// Custom bitmap
void print_icon(int x, int y, const unsigned char icon[]) {
    myGLCD.setColor(menuBtnColor);
    myGLCD.setBackColor(themeBackground);
    int i = 0, row, column, bit, temp;
    int constant = 1;
    for (row = 0; row < 40; row++) {
        for (column = 0; column < 5; column++) {
            temp = icon[i];
            for (bit = 7; bit >= 0; bit--) {

                if (temp & constant) {
                    myGLCD.drawPixel(x + (column * 8) + (8 - bit), y + row);
                }
                else {

                }
                temp >>= 1;
            }
            i++;
        }
    }
}

/***************************************************
*  Draw Round/Square Button                        *
*                                                  *
*  Description:   Draws shapes with/without text   *
*                                                  *
*  Parameters: x start, y start, x stop, y stop    *
*              String: Button text                 *
*              Hex value: Background Color         *
*              Hex value: Border of shape          *
*              Hex value: Color of text            *
*              int: Alignment of text #defined as  *
*                   LEFT, CENTER, RIGHT            *
*                                                  *
***************************************************/
void drawRoundBtn(int x_start, int y_start, int x_stop, int y_stop, String button, int backgroundColor, int btnBorderColor, int btnTxtColor, int align) {
    int size, temp, offset;
    
    myGLCD.setColor(backgroundColor);
    myGLCD.fillRoundRect(x_start, y_start, x_stop, y_stop); // H_Start, V_Start, H_Stop, V_Stop
    myGLCD.setColor(btnBorderColor);
    myGLCD.drawRoundRect(x_start, y_start, x_stop, y_stop);
    myGLCD.setColor(btnTxtColor); // text color
    myGLCD.setBackColor(backgroundColor); // text background
    switch (align)
    {
    case 1:
        myGLCD.print(button, x_start + 5, y_start + ((y_stop - y_start) / 2) - 8); // hor, ver
        break;
    case 2:
        size = button.length();
        temp = ((x_stop - x_start) / 2);
        offset = x_start + (temp - (8 * size));
        myGLCD.print(button, offset, y_start + ((y_stop - y_start) / 2) - 8); // hor, ver
        break;
    case 3:
        // Currently hotwired for deg text only
        myGLCD.print(button, x_start + 55, y_start + ((y_stop - y_start) / 2) - 8); // hor, ver
        break;
    default:
        break;
    }

}

void drawSquareBtn(int x_start, int y_start, int x_stop, int y_stop, String button, int backgroundColor, int btnBorderColor, int btnTxtColor, int align) {
    int size, temp, offset;
    myGLCD.setColor(backgroundColor);
    myGLCD.fillRect(x_start, y_start, x_stop, y_stop); // H_Start, V_Start, H_Stop, V_Stop
    myGLCD.setColor(btnBorderColor);
    myGLCD.drawRect(x_start, y_start, x_stop, y_stop);
    myGLCD.setColor(btnTxtColor); // text color
    myGLCD.setBackColor(backgroundColor); // text background
    switch (align)
    {
    case 1:
        myGLCD.print(button, x_start + 5, y_start + ((y_stop - y_start) / 2) - 8); // hor, ver
        break;
    case 2:
        size = button.length();
        temp = ((x_stop - x_start) / 2);
        offset = x_start + (temp - (8 * size));
        myGLCD.print(button, offset, y_start + ((y_stop - y_start) / 2) - 8); // hor, ver
        break;
    case 3:
        //align left
        break;
    default:
        break;
    }
}

// Highlights round buttons when selected
void waitForIt(int x1, int y1, int x2, int y2)
{
    myGLCD.setColor(themeBackground);
    myGLCD.drawRoundRect(x1, y1, x2, y2);
    while (myTouch.dataAvailable())
        myTouch.read();
    myGLCD.setColor(menuBtnBorder);
    myGLCD.drawRoundRect(x1, y1, x2, y2);
}

// Highlights square buttons when selected
void waitForItRect(int x1, int y1, int x2, int y2)
{
    myGLCD.setColor(themeBackground);
    myGLCD.drawRect(x1, y1, x2, y2);
    while (myTouch.dataAvailable())
        myTouch.read();
    myGLCD.setColor(menuBtnBorder);
    myGLCD.drawRect(x1, y1, x2, y2);
}

// Highlights square buttons when selected and sends CAN message
// This function is used for manual control
void waitForItRect(int x1, int y1, int x2, int y2, int txId, byte data[])
{
    myGLCD.setColor(themeBackground);
    myGLCD.drawRect(x1, y1, x2, y2);
    while (myTouch.dataAvailable())
    {
        can1.sendFrame(txId, data);
        myTouch.read();
        delay(80);
    }
    myGLCD.setColor(menuBtnBorder);
    myGLCD.drawRect(x1, y1, x2, y2);
}


/*=========================================================
    Manual control Functions
===========================================================*/
// Draw the manual control page
void drawManualControl()
{
    // Clear LCD to be written 
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

    // Print arm logo
    print_icon(435, 5, robotarm);

    // Print page title
    drawSquareBtn(180, 10, 400, 45, "Manual Control", themeBackground, themeBackground, menuBtnColor, CENTER);

    // Manual control axis labels
    //myGLCD.setColor(VGA_BLACK);
    //myGLCD.setBackColor(VGA_BLACK);
    //myGLCD.print("Axis", CENTER, 60);
    int j = 1;
    for (int i = 146; i < (480-45); i = i + 54) {
        myGLCD.setColor(menuBtnColor);
        myGLCD.setBackColor(themeBackground);
        myGLCD.printNumI(j, i + 20, 60);
        j++;
    }

    // Draw the upper row of movement buttons
        // x_Start, y_start, x_Stop, y_stop
    for (int i = 146; i < (480 - 54); i = i + 54) {
        drawSquareBtn(i, 80, i + 54, 140, "/\\", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    }

    // Draw the bottom row of movement buttons
    // x_Start, y_start, x_Stop, y_stop
    for (int i = 146; i < (480 - 54); i = i + 54) {
        drawSquareBtn(i, 140, i + 54, 200, "\\/", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    }

    // Draw Select arm buttons
    drawSquareBtn(165, 225, 220, 265, "Arm", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(146, 260, 200, 315, "1", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
    drawSquareBtn(200, 260, 254, 315, "2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

    // Draw grip buttons
    drawSquareBtn(270, 225, 450, 265, "Gripper", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(270, 260, 360, 315, "Open", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(360, 260, 450, 315, "Close", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

return;
}

// Draw page button function
void manualControlButtons()
{
    // Mutiply is a future funtion to allow movement of multiple angles at a time instead of just 1
    int multiply = 1; 

    // Enables revese
    uint8_t reverse = 0x10;

    // CAN message ID and frame
    static uint16_t txId = 0x0A3;
    byte data[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // Physical buttons
    /*
    if (digitalRead(A2) == HIGH)
    {
        // Select arm
        //drawRoundButton(75, 275, 125, 315, "1", themeSelected);
        //drawRoundButton(125, 275, 175, 315, "2", themeColor);
        txId = 0x0A3;
        delay(BUTTON_DELAY);
    }
    if (digitalRead(A1) == HIGH)
    {
        // Select arm
        //drawRoundButton(75, 275, 125, 315, "1", themeColor);
        //drawRoundButton(125, 275, 175, 315, "2", themeSelected);
        txId = 0x0B3;
        delay(BUTTON_DELAY);
    }
    if (digitalRead(A0) == HIGH)
    {
        delay(BUTTON_DELAY);
        return;
    }
    */

    // LCD touch funtions
    if (myTouch.dataAvailable())
    {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        if ((y >= 80) && (y <= 140))
        {
            // A1 Up
            if ((x >= 146) && (x <= 200))
            {
                data[1] = 1 * multiply;
                waitForItRect(146, 80, 200, 140, txId, data);
                data[1] = 0;
            }
            // A2 Up
            if ((x >= 200) && (x <= 254))
            {
                data[2] = 1 * multiply;
                waitForItRect(200, 80, 254, 140, txId, data);
                data[2] = 0;
            }
            // A3 Up
            if ((x >= 254) && (x <= 308))
            {
                data[3] = 1 * multiply;
                waitForItRect(254, 80, 308, 140, txId, data);
                data[3] = 0;
            }
            // A4 Up
            if ((x >= 308) && (x <= 362))
            {
                data[4] = 1 * multiply;
                waitForItRect(308, 80, 362, 140, txId, data);
                data[4] = 0;
            }
            // A5 Up
            if ((x >= 362) && (x <= 416))
            {
                data[5] = 1 * multiply;
                waitForItRect(362, 80, 416, 140, txId, data);
                data[5] = 0;
            }
            // A6 Up
            if ((x >= 416) && (x <= 470))
            {
                data[6] = 1 * multiply;
                waitForItRect(416, 80, 470, 140, txId, data);
                data[6] = 0;
            }
        }
        if ((y >= 140) && (y <= 200))
        {
            // A1 Down
            if ((x >= 156) && (x <= 200))
            {
                data[1] = (1 * multiply) + reverse;
                waitForItRect(146, 140, 200, 200, txId, data);
                data[1] = 0;
            }
            // A2 Down
            if ((x >= 200) && (x <= 254))
            {
                data[2] = (1 * multiply) + reverse;
                waitForItRect(200, 140, 254, 200, txId, data);
                data[2] = 0;
            }
            // A3 Down
            if ((x >= 254) && (x <= 308))
            {
                data[3] = (1 * multiply) + reverse;
                waitForItRect(254, 140, 308, 200, txId, data);
                data[3] = 0;
            }
            // A4 Down
            if ((x >= 308) && (x <= 362))
            {
                data[4] = (1 * multiply) + reverse;
                waitForItRect(308, 140, 362, 200, txId, data);
                data[4] = 0;
            }
            // A5 Down
            if ((x >= 362) && (x <= 416))
            {
                data[5] = (1 * multiply) + reverse;
                waitForItRect(362, 140, 416, 200, txId, data);
                data[5] = 0;
            }
            // A6 Down
            if ((x >= 416) && (x <= 470))
            {
                data[6] = (1 * multiply) + reverse;
                waitForItRect(416, 140, 470, 200, txId, data);
                data[6] = 0;
            }
        }
        if ((y >= 260) && (y <= 315))  
        {
            if ((x >= 146) && (x <= 200)) 
            {
                // Select arm 1
                drawSquareBtn(146, 260, 200, 315, "1", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
                drawSquareBtn(200, 260, 254, 315, "2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
                txId = 0x0A3;
            }
            if ((x >= 200) && (x <= 254))  
            {
                // Select arm 2
                drawSquareBtn(146, 260, 200, 315, "1", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
                drawSquareBtn(200, 260, 254, 315, "2", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
                txId = 0x0B3;
            }
            if ((x >= 270) && (x <= 360))
            {
                // Grip open
                data[7] = 1 * multiply;
                waitForItRect(270, 260, 360, 315, txId, data);
                data[7] = 0;
            }
            if ((x >= 360) && (x <= 450))
            {
                // Grip close
                data[7] = (1 * multiply) + reverse;
                waitForItRect(360, 260, 450, 315, txId, data);
                data[7] = 0;
            }
        }
        }
}


/*=========================================================
                    View page 
===========================================================*/
// Draw the view page
void drawView()
{
    // Clear LCD to be written 
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

    // Print arm logo
    print_icon(435, 5, robotarm);

    // Draw row lables
    for (int start = 35, stop = 75, row = 1; start <= 260; start = start + 45, stop = stop + 45, row++)
    {
        String rowLable = "A" + String(row);
        drawRoundBtn(170, start, 190, stop, rowLable, themeBackground, themeBackground, menuBtnColor, CENTER);
    }

    // Boxes for current arm angles
    uint8_t yStart = 35;
    uint8_t yStop = 75;
    // Arm 1
    drawRoundBtn(310, 5, 415, 40, "Arm2", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawRoundBtn(310, yStart + 0, 415, yStop + 0, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 45, 415, yStop + 45, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 90, 415, yStop + 90, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 135, 415, yStop + 135, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 180, 415, yStop + 180, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 225, 415, yStop + 225, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    // Arm 2
    drawRoundBtn(205, 5, 305, 40, "Arm1", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawRoundBtn(200, yStart + 0, 305, yStop + 0, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 45, 305, yStop + 45, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 90, 305, yStop + 90, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 135, 305, yStop + 135, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 180, 305, yStop + 180, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 225, 305, yStop + 225, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
}

// No buttons, consider adding a refresh mechanism 

/*==========================================================
                    Program Arm
============================================================*/
// Draws scrollable box that contains 10 slots for programs
void drawProgramScroll(int scroll)
{
    // selected position = scroll * position
    // if selected draw different color border
    int y = 50;
    
    for (int i = 0; i < 5; i++)
    {
        drawSquareBtn(150, y, 410, y + 40, aList[i + scroll], menuBackground, menuBtnBorder, menuBtnText, LEFT);
        y = y + 40;
        //scroll++;
    }
}

// Draws buttons for program function
void drawProgram(int scroll = 0)
{
    // Clear LCD to be written
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

    // Print arm logo
    print_icon(435, 5, robotarm);

    // Print page title
    drawSquareBtn(180, 10, 400, 45, "Program", themeBackground, themeBackground, menuBtnColor, CENTER);
    
    // Scroll buttons
    myGLCD.setColor(menuBtnColor);
    myGLCD.setBackColor(themeBackground);
    drawSquareBtn(420, 100, 470, 150, "/\\", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(420, 150, 470, 200, "\\/", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

    // Draws program scroll box with current scroll value
    drawProgramScroll(scroll);

    // Draw program buttons
    drawSquareBtn(150, 260, 250, 300, "Open", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(255, 260, 355, 300, "Load", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(360, 260, 460, 300, "Delete", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// Deletes current selected program from 
void programDelete()
{
    sdCard.deleteFile(aList[selectedProgram]);
}

// Load selected program from SD card into linked list
void loadProgram()
{
    sdCard.readFile(aList[selectedProgram], runList);
}

// Executes program currently loaded into linked list
void programRun()
{
    // Was this to clear and leftover messages in buffer?
    can1.getFrameID();

    // Bool control for while loop
    bool isWait = true;

    // Make sure a program was loaded to run
    if (programLoaded == false)
    {
        return;
    }

    // CAN messages for axis movements
    uint8_t bAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t tAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t exeMove[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };    

    
    for (uint8_t i = 0; i < runList.size(); i++)
    {
        // Populate CAN messages with angles from current linkedlist

        // Axis 1
        if (runList.get(i)->getA1() <= 0xFF)
        {
            bAxis[3] = runList.get(i)->getA1();
        }
        else
        {
            bAxis[2] = runList.get(i)->getA1() - 0xFF;
            bAxis[3] = 0xFF;
        }

        // Axis 2
        if (runList.get(i)->getA2() <= 0xFF)
        {
            bAxis[5] = runList.get(i)->getA2();
        }
        else
        {
            bAxis[4] = runList.get(i)->getA2() - 0xFF;
            bAxis[5] = 0xFF;
        }

        // Axis 3
        if (runList.get(i)->getA3() <= 0xFF)
        {
            bAxis[7] = runList.get(i)->getA3();
        }
        else
        {
            bAxis[6] = runList.get(i)->getA3() - 0xFF;
            bAxis[7] = 0xFF;
        }

        // Axis 4
        if (runList.get(i)->getA4() <= 0xFF)
        {
            tAxis[3] = runList.get(i)->getA4();
        }
        else
        {
            tAxis[2] = runList.get(i)->getA4() - 0xFF;
            tAxis[3] = 0xFF;
        }

        // Axis 5
        if (runList.get(i)->getA5() <= 0xFF)
        {
            tAxis[5] = runList.get(i)->getA5();
        }
        else
        {
            tAxis[4] = runList.get(i)->getA5() - 0xFF;
            tAxis[5] = 0xFF;
        }

        // Axis 6
        if (runList.get(i)->getA5() <= 0xFF)
        {
            tAxis[7] = runList.get(i)->getA6();
        }
        else
        {
            tAxis[6] = runList.get(i)->getA6() - 0xFF;
            tAxis[7] = 0xFF;
        }

        // Change to array of IDs
        uint8_t ID = runList.get(i)->getID();

        //Need a way to detect change or know what the current state is before sending
        runList.get(i)->getGrip();

        // Should there be a timeout here?

        // Delay needed because Mega2560 with MCP2515 is slow
        delay(10);

        // Send first frame with axis 1-3
        can1.sendFrame(0x0A1, bAxis);
        while (isWait)
        {
            if (can1.msgCheck(0x0C1, 0x01, 0x01))
            {
                isWait = false;
            }
        }
        isWait = true;

        // Delay needed because Mega2560 with MCP2515 is slow
        delay(10);

        // Send second frame with axis 4-6
        can1.sendFrame(0x0A2, tAxis);
        while (isWait)
        {
            if (can1.msgCheck(0x0C1, 0x02, 0x01))
            {
                isWait = false;
            }
        }
        isWait = true;

        // Delay needed because Mega2560 with MCP2515 is slow
        delay(10);

        // Send third frame with grip and execute command
        can1.sendFrame(0x0A0, exeMove);
        while (isWait)
        {
            if (can1.msgCheck(0x0C1, 0x03, 0x01))
            {
                isWait = false;
            }
        }
        isWait = true;
    }
}

// Button functions for program page 
void programButtons()
{
    // Static so that the position is saved while this method is repeatedly called in a loop
    static int scroll = 0;

    // Touch screen controls
    if (myTouch.dataAvailable())
    {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        if ((x >= 150) && (x <= 410))
        {
            if ((y >= 50) && (y <= 90))
            {
                // 1st position of scroll box
                waitForItRect(150, 50, 410, 90);
                Serial.println(1 + scroll);
                selectedProgram = 0 + scroll;
            }
            if ((y >= 90) && (y <= 130))
            {
                // 2nd position of scroll box
                waitForItRect(150, 90, 410, 130);
                Serial.println(2 + scroll);
                selectedProgram = 1 + scroll;
            }
            if ((y >= 130) && (y <= 170))
            {
                // 3d position of scroll box
                waitForItRect(150, 130, 410, 170);
                Serial.println(3 + scroll);
                selectedProgram = 2 + scroll;
            }
            if ((y >= 170) && (y <= 210))
            {
                // 4th position of scroll box
                waitForItRect(150, 170, 410, 210);
                Serial.println(4 + scroll);
                selectedProgram = 3 + scroll;
            }
            if ((y >= 210) && (y <= 250))
            {
                // 5th position of scroll box
                waitForItRect(150, 210, 410, 250);
                Serial.println(5 + scroll);
                selectedProgram = 4 + scroll;
            }
        }
        if ((x >= 420) && (x <= 470))
        {
            if ((y >= 100) && (y <= 150))
            {
                // Scroll up
                waitForIt(420, 100, 470, 150);
                if (scroll > 0)
                {
                    scroll--;
                    drawProgramScroll(scroll);
                }
            }
        }
        if ((x >= 420) && (x <= 470))
        {
            if ((y >= 150) && (y <= 200))
            {
                // Scroll down
                waitForIt(420, 150, 470, 200);
                if (scroll < 5)
                {
                    scroll++;
                    drawProgramScroll(scroll);
                }
            }
        }

        if ((y >= 260) && (y <= 300))
        {
            if ((x >= 150) && (x <= 250))
            {
                // Open program
                waitForItRect(150, 260, 250, 300);
                runList.clear();
                loadProgram();
                programOpen = true;
                pageControl(6, 0);
            }
            if ((x >= 255) && (x <= 355))
            {
                // Load program
                waitForItRect(255, 260, 355, 300);
                runList.clear();
                loadProgram();

            }
            if ((x >= 360) && (x <= 460))
            {
                // Delete program
                waitForItRect(360, 260, 460, 300);
                errorMSG("Error", "Delete File?", "Cannot be undone");
                programDelete();
            }
        }
    }
    return;
}


/*==========================================================
                    Edit Program 
============================================================*/
// Draws scrollable box that contains all the nodes in a program
void drawProgramEditScroll(uint8_t scroll = 0)
{
    myGLCD.setFont(SmallFont);
    uint8_t nodeSize = runList.size();
    Serial.println(nodeSize);
    // Each node should be listed with all information, might need small text
    int row = 50;
    for (int i = 0; i < 5; i++)
    {
        String position = String(i + scroll);
        String a = ":";
        String b = " ";
        String label = position + a + String(runList.get(i + scroll)->getA1()) + b + String(runList.get(i + scroll)->getA2())
            + b + String(runList.get(i + scroll)->getA3()) + b + String(runList.get(i + scroll)->getA4()) + b + String(runList.get(i + scroll)->getA5())
            + b + String(runList.get(i + scroll)->getA6()) + b + String(runList.get(i + scroll)->getGrip()) + b + String(runList.get(i + scroll)->getID());
        if (i + scroll < nodeSize)
        {
            drawSquareBtn(150, row, 410, row + 40, label, menuBackground, menuBtnBorder, menuBtnText, LEFT);
        }
        else
        {
            drawSquareBtn(150, row, 410, row + 40, "", menuBackground, menuBtnBorder, menuBtnText, LEFT);
        }
        
        row = row + 40;
        //scroll++;
    }
    // Load linked list from SD card unless already in linked list
    // Need some gui to edit, add and instert nodes
    // Save button will write to SD card and return
    // cancel button will return without saving
    // 5 buttons in total

    // To edit a node just replace with a new position
    myGLCD.setFont(BigFont);
}

// Draws buttons for edit program function
void drawProgramEdit(uint8_t scroll = 0)
{
    // Clear LCD to be written
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

    // Print arm logo
    print_icon(435, 5, robotarm);

    // Print page title
    drawSquareBtn(180, 10, 400, 45, "Edit", themeBackground, themeBackground, menuBtnColor, CENTER);

    // Scroll buttons
    myGLCD.setColor(menuBtnColor);
    myGLCD.setBackColor(themeBackground);
    drawSquareBtn(420, 100, 470, 150, "/\\", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(420, 150, 470, 200, "\\/", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

    // Draw program edit buttons
    drawSquareBtn(150, 260, 230, 310, "Add", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(230, 260, 310, 310, "Ins", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(310, 260, 390, 310, "Del", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(390, 260, 470, 310, "Save", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

    drawSquareBtn(420, 50, 470, 90, "X", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// Adds current position to program linked list 
void addNode(int insert = -1, bool grip = false, uint8_t channel = 1)
{
    // Array of arm axis positions
    uint16_t posArray[8] = { 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000 };

    // Request and update 
    axisPos.updateAxisPos();

    // Update array value with data collected from the axis position update
    posArray[0] = axisPos.getA1C1();
    posArray[1] = axisPos.getA2C1();
    posArray[2] = axisPos.getA3C1();
    posArray[3] = axisPos.getA4C1();
    posArray[4] = axisPos.getA5C1();
    posArray[5] = axisPos.getA6C1();

    // Create program object with array positions, grip on/off, and channel
    Program* node = new Program(posArray, grip, channel);

    if (insert < 0)
    {
        // Add created object to linked list
        runList.add(node);
    }
    else
    {
        runList.add(insert, node);
    }
   
}

void deleteNode(uint16_t nodeLocation)
{
    runList.remove(nodeLocation);
}

// Insert current position to program linked list
void insertNode()
{

}

// Writes current selected linked list to SD card
void saveProgram()
{
    // Delimiter 
    String space = ", ";

    // Write out linkedlist data to text file
    for (uint8_t i = 0; i < runList.size(); i++)
    {
        sdCard.writeFile(aList[selectedProgram], ",");
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA1());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA2());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA3());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA4());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA5());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getA6());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getID());
        sdCard.writeFile(aList[selectedProgram], space);
        sdCard.writeFile(aList[selectedProgram], runList.get(i)->getGrip());
        sdCard.writeFileln(aList[selectedProgram]);
    }
}

// Button functions for edit program page
void programEditButtons()
{
    static int scroll = 0;
    // Touch screen controls
    if (myTouch.dataAvailable())
    {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        if ((x >= 150) && (x <= 410))
        {
            if ((y >= 50) && (y <= 90))
            {
                waitForItRect(150, 50, 410, 90);
                Serial.println(1 + scroll);
                selectedNode = 0 + scroll;
                drawProgramEditScroll(scroll);
            }
            if ((y >= 90) && (y <= 130))
            {
                waitForItRect(150, 90, 410, 130);
                Serial.println(2 + scroll);
                selectedNode = 1 + scroll;
                drawProgramEditScroll(scroll);
            }
            if ((y >= 130) && (y <= 170))
            {
                waitForItRect(150, 130, 410, 170);
                Serial.println(3 + scroll);
                selectedNode = 2 + scroll;
                drawProgramEditScroll(scroll);
            }
            if ((y >= 170) && (y <= 210))
            {
                waitForItRect(150, 170, 410, 210);
                Serial.println(4 + scroll);
                selectedNode = 3 + scroll;
                drawProgramEditScroll(scroll);
            }
            if ((y >= 210) && (y <= 250))
            {
                waitForItRect(150, 210, 410, 250);
                Serial.println(5 + scroll);
                selectedNode = 4 + scroll;
                drawProgramEditScroll(scroll);
            }
        }
        if ((x >= 420) && (x <= 470))
        {
            if ((y >= 50) && (y <= 90))
            {
                // Cancel
                waitForItRect(420, 50, 470, 90);
                programOpen = false;
                pageControl(2, false);
            }
            if ((y >= 100) && (y <= 150))
            {
                waitForIt(420, 100, 470, 150);
                if (scroll > 0)
                {
                    scroll--;
                    drawProgramEditScroll(scroll);
                }
            }
            if ((y >= 150) && (y <= 200))
            {
                waitForIt(420, 150, 470, 200);
                if (scroll < 10)
                {
                    scroll++;
                    drawProgramEditScroll(scroll);
                }
            }
        }

        if ((y >= 260) && (y <= 310))
        {
            if ((x >= 150) && (x <= 230))
            {
                // Add node
                waitForItRect(150, 260, 230, 310);
                addNode();
                drawProgramEditScroll(scroll);
            }
            if ((x >= 230) && (x <= 310))
            {
                // Insert node
                waitForItRect(230, 260, 310, 310);
                addNode(selectedNode);
                drawProgramEditScroll(scroll);
            }
            if ((x >= 310) && (x <= 390))
            {
                // Delete node
                waitForItRect(310, 260, 390, 310);
                deleteNode(selectedNode);
                drawProgramEditScroll(scroll);
            }
            if ((x >= 390) && (x <= 470))
            {
                // Save program
                waitForItRect(390, 260, 470, 310);
                programDelete();
                saveProgram();
                programOpen = false;
                pageControl(2, false);
            }
        }
    }
    return;
}


/*==========================================================
                    Configure Arm
============================================================*/
// Draws the config page
void drawConfig()
{
    drawSquareBtn(180, 10, 400, 45, "Configuration", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);
    drawSquareBtn(180, 10, 400, 45, "Configuration", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawRoundBtn(150, 60, 300, 100, "Home Ch1", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(310, 60, 460, 100, "Set Ch1", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(150, 110, 300, 150, "Home Ch2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(310, 110, 460, 150, "Set ch2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    return;
}

// Sends command to return arm to starting position
void homeArm(uint8_t* armIDArray)
{
    byte data1[8] = { 0x00, 0x00, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0x5A };
    byte data2[8] = { 0x00, 0x00, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4 };
    byte data3[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    can1.sendFrame(armIDArray[0], data1);
    delay(130);
    can1.sendFrame(armIDArray[1], data2);
    delay(130);
    can1.sendFrame(armIDArray[2], data3);
}

// Button functions for config page
void configButtons()
{
    uint8_t setHomeId[8] = { 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t* setHomeIdPtr = setHomeId;

    uint8_t arm1IDArray[3] = { 0x0A1, 0x0A2, 0x0A0 };
    uint8_t arm2IDArray[3] = { 0x0B1, 0x0B2, 0x0B0 };
    uint8_t* arm1IDPtr = arm1IDArray;
    uint8_t* arm2IDPtr = arm2IDArray;
    // Touch screen controls
    if (myTouch.dataAvailable())
    {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        if ((y >= 60) && (y <= 100))
        {
            if ((x >= 150) && (x <= 300))
            {
                waitForIt(150, 60, 300, 100);
                homeArm(arm1IDPtr);
            }
            if ((x >= 310) && (x <= 460))
            {
                waitForIt(310, 60, 460, 100);
                can1.sendFrame(arm1IDArray[2], setHomeIdPtr);
            }
        }
        if ((y >= 110) && (y <= 150))
        {
            if ((x >= 150) && (x <= 300))
            {
                waitForIt(150, 110, 300, 150);
                homeArm(arm2IDPtr);
            }
            if ((x >= 310) && (x <= 460))
            {
                waitForIt(310, 110, 460, 150);
                can1.sendFrame(arm2IDArray[2], setHomeIdPtr);
            }
        }
    }
    return;
}


/*=========================================================
    Framework Functions
===========================================================*/
//Only called once at startup to draw the menu
void drawMenu()
{
    // Draw Layout
    drawSquareBtn(1, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    drawSquareBtn(1, 1, 140, 319, "", menuBackground, menuBackground, menuBackground, CENTER);
    
    // Draw Menu Buttons
    drawRoundBtn(10, 10, 130, 65, "1-VIEW", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(10, 70, 130, 125, "2-PROG", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(10, 130, 130, 185, "3-MOVE", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(10, 190, 130, 245, "4-CONF", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawRoundBtn(10, 250, 130, 305, "5-EXEC", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
  
    can1.startCAN();
    bool hasFailed = sdCard.startSD();
    if (!hasFailed)
    {
        Serial.println("SD failed");
    }
    else if (hasFailed)
    {
        Serial.println("SD Running");
    }
   
    myGLCD.InitLCD();
    myGLCD.clrScr();

    myTouch.InitTouch();
    myTouch.setPrecision(PREC_MEDIUM);

    myGLCD.setFont(BigFont);
    myGLCD.setBackColor(0, 0, 255);

    drawMenu();
}

// Page control framework
void pageControl(int page, bool value = false)
{
    // Static bool ensures the page is drawn only once while the loop is running
    static bool hasDrawn;
    // Seperated because compiler produces error with 1 line
    hasDrawn = value;
    
    while (true)
    {
        // Check if button on menu is pushed
        menuButtons();

        // Switch which page to load
        switch (page)
        {
        case 1:
            // Draw page
            if (!hasDrawn)
            {
                drawView();
                axisPos.drawAxisPos(myGLCD);
                hasDrawn = true;
                controlPage = page;
            }
            // Call buttons if any
            break;
        case 2:
            if (programOpen)
            {
                pageControl(6);
            }
            // Draw page
            if (!hasDrawn)
            {
                drawProgram();
                hasDrawn = true;
                controlPage = page;
            }
            // Call buttons if any
            programButtons();
            break;
        case 3:
            // Draw page
            if (!hasDrawn)
            {
                drawManualControl();
                hasDrawn = true;
                controlPage = page;
            }
            // Call buttons if any
            manualControlButtons();
            break;
        case 4:
            // Draw page
            if (!hasDrawn)
            {
                drawConfig();
                hasDrawn = true;
                controlPage = page;
            }
            // Call buttons if any
            configButtons();
            break;
        case 5:
            // Draw page
            if (!hasDrawn)
            {
                hasDrawn = true;
                programLoaded = true;
                programRun();
                pageControl(controlPage, true);
            }
            page = 2;
            // Call buttons if any
            break;
        case 6:
            // Draw page
            if (!hasDrawn)
            {
                hasDrawn = true;
                programLoaded = true;
                drawProgramEdit();
                drawProgramEditScroll();
                controlPage = page;
            }
            // Call buttons if any
            programEditButtons();
            break;
        }

    }
}

// Error Message function
void errorMSG(String title, String eMessage1, String eMessage2)
{
    drawSquareBtn(170, 140, 450, 240, "", menuBackground, menuBtnColor, menuBtnColor, CENTER);
    drawSquareBtn(170, 140, 450, 170, title, themeBackground, menuBtnColor, menuBtnBorder, LEFT);
    drawSquareBtn(171, 171, 449, 204, eMessage1, menuBackground, menuBackground, menuBtnText, CENTER);
    drawSquareBtn(171, 205, 449, 239, eMessage2, menuBackground, menuBackground, menuBtnText, CENTER);
    drawRoundBtn(400, 140, 450, 170, "X", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
}

// Error message without confirmation
uint8_t errorMSGBtn(uint8_t page)
{
    // Touch screen controls
    if (myTouch.dataAvailable())
    {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        if ((x >= 400) && (x <= 450))
        {
            if ((y >= 140) && (y <= 170))
            {
                waitForItRect(400, 140, 450, 170);
                pageControl(page);
            }
        }
    }
}

// Buttons for the main menu
void menuButtons()
{
    while (true)
    {

        // Physical Button control
        if (digitalRead(A4) == HIGH)
        {
            //drawLoadProgram();
            drawMenu();
            delay(BUTTON_DELAY);
        }
        if (digitalRead(A3) == HIGH)
        {
            //drawManualControl();
            delay(BUTTON_DELAY);
            drawMenu();
        }
        if (digitalRead(A2) == HIGH)
        {
            //drawSettings();
            delay(BUTTON_DELAY);
            drawMenu();
        }
        if (digitalRead(A1) == HIGH)
        {
            //runProgram(*programPtr);
            delay(BUTTON_DELAY);
            drawMenu();
        }
        if (digitalRead(A0) == HIGH)
        {
            delay(BUTTON_DELAY);
        }

        // Touch screen controls
        if (myTouch.dataAvailable())
        {
            myTouch.read();
            x = myTouch.getX();
            y = myTouch.getY();

            // Menu
            if ((x >= 10) && (x <= 130))  // Button: 1
            {
                if ((y >= 10) && (y <= 65))  // Upper row
                {
                    waitForIt(10, 10, 130, 65);
                    pageControl(1);
                }
                if ((y >= 70) && (y <= 125))  // Upper row
                {

                    // X_Start, Y_Start, X_Stop, Y_Stop
                    waitForIt(10, 70, 130, 125);
                    pageControl(2);

                }
                if ((y >= 130) && (y <= 185))  // Upper row
                {
                    // X_Start, Y_Start, X_Stop, Y_Stop
                    waitForIt(10, 130, 130, 185);
                    pageControl(3);
                }
                // Settings touch button
                if ((y >= 190) && (y <= 245))
                {

                    // X_Start, Y_Start, X_Stop, Y_Stop
                    waitForIt(10, 190, 130, 245);
                    pageControl(4);
                }
                if ((y >= 250) && (y <= 305))
                {

                    // X_Start, Y_Start, X_Stop, Y_Stop
                    waitForIt(10, 250, 130, 305);
                    pageControl(5);
                }

            }
        }
        return;
    }
}

// Calls pageControl with a value of 1 to set view page as the home page
void loop() 
{
    pageControl(controlPage);
}