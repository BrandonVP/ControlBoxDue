/*
 Name:    ControlBoxDue.ino
 Created: 11/15/2020 8:27:18 AM
 Author:  Brandon Van Pelt
*/

#include <LinkedList.h>
#include <SD.h>
#include <UTouchCD.h>
#include <memorysaver.h>
#include <SPI.h>
#include <UTFT.h>
#include <UTouch.h>
#include "AxisPos.h"
#include "CANBus.h"
#include "definitions.h"
#include "icons.h"
#include "SDCard.h"
#include "Program.h"


// Initialize display
//(byte model, int RS, int WR, int CS, int RST, int SER)
UTFT myGLCD(ILI9488_16, 7, 38, 9, 10);    
//RTP: byte tclk, byte tcs, byte din, byte dout, byte irq
UTouch  myTouch(2, 6, 3, 4, 5);      

// Objects for keeping track of current angle positions for both arms
AxisPos arm1;
AxisPos arm2;

// For touch controls
int x, y;

// External import for fonts
// extern uint8_t SmallFont[];
extern uint8_t BigFont[];

// Object to control CAN Bus hardware
CANBus can1;

// Object to control SD Card Hardware
SDCard sdCard;

AxisPos axisPos;

LinkedList<Program*> runList = LinkedList<Program*>();



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

void waitForIt(int x1, int y1, int x2, int y2)
{
    myGLCD.setColor(themeBackground);
    myGLCD.drawRoundRect(x1, y1, x2, y2);
    while (myTouch.dataAvailable())
        myTouch.read();
    myGLCD.setColor(menuBtnBorder);
    myGLCD.drawRoundRect(x1, y1, x2, y2);
}

void waitForItRect(int x1, int y1, int x2, int y2)
{
    myGLCD.setColor(themeBackground);
    myGLCD.drawRect(x1, y1, x2, y2);
    while (myTouch.dataAvailable())
        myTouch.read();
    myGLCD.setColor(menuBtnBorder);
    myGLCD.drawRect(x1, y1, x2, y2);
}

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
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);
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

    // Draw the upper row of buttons
        // x_Start, y_start, x_Stop, y_stop
    for (int i = 146; i < (480 - 54); i = i + 54) {
        drawSquareBtn(i, 80, i + 54, 140, "/\\", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    }

    // Draw the bottom row of buttons
    // x_Start, y_start, x_Stop, y_stop
    for (int i = 146; i < (480 - 54); i = i + 54) {
        drawSquareBtn(i, 140, i + 54, 200, "\\/", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    }

    // Draw Select arm
    drawSquareBtn(165, 225, 220, 265, "Arm", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(146, 260, 200, 315, "1", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
    drawSquareBtn(200, 260, 254, 315, "2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

    // Draw grip buttons
    drawSquareBtn(270, 225, 450, 265, "Gripper", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(270, 260, 360, 315, "Open", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(360, 260, 450, 315, "Close", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

return;
}

void manualControlBtns()
{
    int multiply = 1; // MOVE ME or something
    static uint16_t txId = 0x0A3;
    uint8_t reverse = 0x10;
    byte data[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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
        if (myTouch.dataAvailable())
        {
            myTouch.read();
            x = myTouch.getX();
            y = myTouch.getY();

            if ((y >= 80) && (y <= 140))
            {
                // A1 Up
                // 30, 110, 90, 170
                if ((x >= 146) && (x <= 200))
                {
                    data[1] = 1 * multiply;
                    waitForItRect(146, 80, 200, 140, txId, data);
                    data[1] = 0;
                }
                // A2 Up
                // 90, 110, 150, 170
                if ((x >= 200) && (x <= 254))
                {
                    data[2] = 1 * multiply;
                    waitForItRect(200, 80, 254, 140, txId, data);
                    data[2] = 0;
                }
                // A3 Up
                // 150, 110, 210, 170
                if ((x >= 254) && (x <= 308))
                {
                    data[3] = 1 * multiply;
                    waitForItRect(254, 80, 308, 140, txId, data);
                    data[3] = 0;
                }
                // A4 Up
                // 210, 110, 270, 170
                if ((x >= 308) && (x <= 362))
                {
                    data[4] = 1 * multiply;
                    waitForItRect(308, 80, 362, 140, txId, data);
                    data[4] = 0;
                }
                // A5 Up
                // 270, 110, 330, 170
                if ((x >= 362) && (x <= 416))
                {
                    data[5] = 1 * multiply;
                    waitForItRect(362, 80, 416, 140, txId, data);
                    data[5] = 0;
                }
                // A6 Up
                // 330, 110, 390, 170
                if ((x >= 416) && (x <= 470))
                {
                    data[6] = 1 * multiply;
                    waitForItRect(416, 80, 470, 140, txId, data);
                    data[6] = 0;
                }
            }
            // A1 Down
            // 30, 170, 90, 230
            if ((y >= 140) && (y <= 200))
            {
                if ((x >= 156) && (x <= 200))
                {
                    data[1] = (1 * multiply) + reverse;
                    waitForItRect(146, 140, 200, 200, txId, data);
                    data[1] = 0;
                }
                // A2 Down
                // 90, 170, 150, 230
                if ((x >= 200) && (x <= 254))
                {
                    data[2] = (1 * multiply) + reverse;
                    waitForItRect(200, 140, 254, 200, txId, data);
                    data[2] = 0;
                }
                // A3 Down
                // 150, 170, 210, 230
                if ((x >= 254) && (x <= 308))
                {
                    data[3] = (1 * multiply) + reverse;
                    waitForItRect(254, 140, 308, 200, txId, data);
                    data[3] = 0;
                }
                // A4 Down
                // 210, 170, 270, 230
                if ((x >= 308) && (x <= 362))
                {
                    data[4] = (1 * multiply) + reverse;
                    waitForItRect(308, 140, 362, 200, txId, data);
                    data[4] = 0;
                }
                // A5 Down
                // 270, 170, 330, 230
                if ((x >= 362) && (x <= 416))
                {
                    data[5] = (1 * multiply) + reverse;
                    waitForItRect(362, 140, 416, 200, txId, data);
                    data[5] = 0;
                }
                // A6 Down
                // 330, 170, 390, 230
                if ((x >= 416) && (x <= 470))
                {
                    data[6] = (1 * multiply) + reverse;
                    waitForItRect(416, 140, 470, 200, txId, data);
                    data[6] = 0;
                }
            }

            if ((y >= 260) && (y <= 315))  // Upper row
            {
                if ((x >= 146) && (x <= 200))  // Button: 1
                {
                    // Select arm
                    drawSquareBtn(146, 260, 200, 315, "1", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
                    drawSquareBtn(200, 260, 254, 315, "2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
                    txId = 0x0A3;
                }
                if ((x >= 200) && (x <= 254))  // Button: 1
                {
                    drawSquareBtn(146, 260, 200, 315, "1", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
                    drawSquareBtn(200, 260, 254, 315, "2", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
                    txId = 0x0B3;
                }
                if ((x >= 270) && (x <= 360))  // Button: 1
                {
                    data[7] = 1 * multiply;
                    waitForItRect(270, 260, 360, 315, txId, data);
                    data[7] = 0;
                }
                if ((x >= 360) && (x <= 450))  // Button: 1
                {
                    data[7] = (1 * multiply) + reverse;
                    waitForItRect(360, 260, 450, 315, txId, data);
                    data[7] = 0;
                }
            }
        }
}


/*=========================================================
    View page Functions
===========================================================*/
// Draw the view page
void drawView()
{
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);
    // Draw body
    for (int start = 35, stop = 75, row = 1; start <= 260; start = start + 45, stop = stop + 45, row++)
    {
        String a = "A" + String(row);
        drawRoundBtn(170, start, 190, stop, a, themeBackground, themeBackground, menuBtnColor, CENTER);
    }

    uint8_t yStart = 35;
    uint8_t yStop = 75;
    drawRoundBtn(310, 5, 415, 40, "Arm2", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawRoundBtn(310, yStart + 0, 415, yStop + 0, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 45, 415, yStop + 45, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 90, 415, yStop + 90, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 135, 415, yStop + 135, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 180, 415, yStop + 180, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(310, yStart + 225, 415, yStop + 225, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);


    drawRoundBtn(205, 5, 305, 40, "Arm1", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawRoundBtn(200, yStart + 0, 305, yStop + 0, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 45, 305, yStop + 45, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 90, 305, yStop + 90, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 135, 305, yStop + 135, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 180, 305, yStop + 180, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    drawRoundBtn(200, yStart + 225, 305, yStop + 225, "deg", menuBackground, menuBackground, menuBtnColor, RIGHT);
    axisPos.drawAxisPos(myGLCD);
}

/*==========================================================
                    Program Arm
============================================================*/
void drawProgramScroll(int scroll)
{
    // This array should populate from SD CARD
    String alist[10] = { "1-Axis Test", "2-Demo", "3-Empty", "4-Empty", "5-Empty", "6-Empty", "7-Empty", "8-Empty", "9-Empty", "10-Empty" };

    int y = 50;
    for (int i = 0; i < 5; i++)
    {
        drawSquareBtn(150, y, 410, y + 40, alist[scroll], menuBackground, menuBtnBorder, menuBtnText, LEFT);
        y = y + 40;
        scroll++;
    }
}

void drawProgram(int scroll = 0)
{
    
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    drawSquareBtn(180, 10, 400, 45, "Program", themeBackground, themeBackground, menuBtnColor, CENTER);
    print_icon(435, 5, robotarm);
    int j = 50;
    int k = scroll;
    myGLCD.setColor(menuBtnColor);
    myGLCD.setBackColor(themeBackground);
    drawSquareBtn(420, 100, 470, 150, "/\\", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(420, 150, 470, 200, "\\/", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawProgramScroll(scroll);
    drawSquareBtn(150, 260, 250, 300, "Create", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(255, 260, 355, 300, "Edit", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    drawSquareBtn(360, 260, 460, 300, "Delete", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
    
}

void addNode(bool grip = false)
{
    uint8_t posArray[8] = { 0x00, 0x00, 0x00, 0x0, 0x00, 0x00, 0x00, 000 };
    axisPos.updateAxisPos();
    posArray[0] = axisPos.getA1C1();
    posArray[1] = axisPos.getA2C1();
    posArray[2] = axisPos.getA3C1();
    posArray[3] = axisPos.getA4C1();
    posArray[4] = axisPos.getA5C1();
    posArray[5] = axisPos.getA6C1();
    Program* node = new Program(posArray, grip);
    runList.add(node);
}

void saveProgram(char* filename)
{
    //SW.write file
    // for linkedlist
    // write to SD
}

void programRun()
{
    int temp = runList.size();

    //sendFrame(uint16_t id, byte * frame)
    // Program class creates an object to store a move
    //  - Needs Address ID, and 6 positions plus grip (8) 
    // Linked list creates list of objects from program class
    // SD card function to save entire linkedlist to SD card
    // function to import linkedlist from SD card
    // Dont need to edit or read line by line from SD, instead delete old file and save entire list when needed
}

void programEdit(int linkPos)
{
    //Loads list into linkedlist
    
    // for (EOF)
    //{
    // nextline = array
    // runList.add(new object(array))
    //}
}

void programDelete(char * filename)
{
    // SD.delete file
}

void program()
{
    static int test = 0;
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
                }
                if ((y >= 90) && (y <= 130))
                {
                    waitForItRect(150, 90, 410, 130);
                    Serial.println(2 + scroll);
                }
                if ((y >= 130) && (y <= 170))
                {
                    waitForItRect(150, 130, 410, 170);
                    Serial.println(3 + scroll);
                }
                if ((y >= 170) && (y <= 210))
                {
                    waitForItRect(150, 170, 410, 210);
                    Serial.println(4 + scroll);
                }
                if ((y >= 210) && (y <= 250))
                {
                    waitForItRect(150, 210, 410, 250);
                    Serial.println(5 + scroll);
                }
            }
            if ((x >= 420) && (x <= 470))  
            {
                if ((y >= 100) && (y <= 150)) 
                {
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
                    waitForItRect(150, 260, 250, 300);
                    addNode();
                }
                if ((x >= 255) && (x <= 355))
                {
                    waitForItRect(255, 260, 355, 300);
                    uint8_t bAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                    uint8_t tAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                    uint8_t execMove[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

                    
                    bAxis[2] = runList.get(0)->getA1();
                    bAxis[4] = runList.get(0)->getA2();
                    bAxis[7] = runList.get(0)->getA3();
                    tAxis[2] = runList.get(0)->getA4();
                    tAxis[4] = runList.get(0)->getA5();
                    tAxis[7] = runList.get(0)->getA6();
                    

                    can1.sendFrame(0x0A1, bAxis);
                    delay(500);
                    can1.sendFrame(0x0A2, tAxis);
                    delay(1000);
                    can1.sendFrame(0x0A0, execMove);
                    
                    
                }
                if ((x >= 360) && (x <= 460))
                {
                    waitForItRect(360, 260, 460, 300);
                    uint8_t bAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                    uint8_t tAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                    uint8_t execMove[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


                    bAxis[2] = runList.get(1)->getA1();
                    bAxis[4] = runList.get(1)->getA2();
                    bAxis[7] = runList.get(1)->getA3();
                    tAxis[2] = runList.get(1)->getA4();
                    tAxis[4] = runList.get(1)->getA5();
                    tAxis[7] = runList.get(1)->getA6();


                    can1.sendFrame(0x0A1, bAxis);
                    delay(500);
                    can1.sendFrame(0x0A2, tAxis);
                    delay(1000);
                    can1.sendFrame(0x0A0, execMove);
                }
            }
        }
        return;
}

/*==========================================================
                    Configure Arm
============================================================*/
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

void config()
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
    sdCard.writeFile("off");
    sdCard.writeFileln();
   

    myGLCD.InitLCD();
    myGLCD.clrScr();

    myTouch.InitTouch();
    myTouch.setPrecision(PREC_MEDIUM);

    myGLCD.setFont(BigFont);
    myGLCD.setBackColor(0, 0, 255);


    drawMenu();
}

void pageControl(int page, bool value = false)
{
    byte test2[8] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22 };
    typedef byte test[8];
    test a1;
    int a2;
    
    static bool hasDrawn;
    static bool isExecute;
    hasDrawn = value;
    while (true)
    {
        menu();
        if (isExecute)
        {

        }
        switch (page)
        {
        case 1:
            if (!hasDrawn)
            {
                drawView();
                //arm1.updateAxisPos();
                hasDrawn = true;
            }
            // Call buttons if any
            break;
        case 2:
            if (!hasDrawn)
            {
                drawProgram();
                hasDrawn = true;
            }
            // Call buttons if any
            program();
            break;
        case 3:
            if (!hasDrawn)
            {
                drawManualControl();
                hasDrawn = true;
            }
            // Call buttons if any
            manualControlBtns();
            break;
        case 4:
            if (!hasDrawn)
            {
                drawConfig();
                hasDrawn = true;
            }
            // Call buttons if any
            config();
            break;
        case 5:
            if (!hasDrawn)
            {
                can1.getFrame(0xA1);
                hasDrawn = true;
            }
            // Call buttons if any
            break;
        }
    }
}

void menu()
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
void loop() {

    pageControl(1);
}