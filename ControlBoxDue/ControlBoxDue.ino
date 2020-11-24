/*
 Name:    ControlBoxDue.ino
 Created: 11/15/2020 8:27:18 AM
 Author:  brand
*/

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

// Initialize display
UTFT myGLCD(ILI9488_16, 7, 38, 9, 10);    //(byte model, int RS, int WR, int CS, int RST, int SER)
UTouch  myTouch(2, 6, 3, 4, 5);      //RTP: byte tclk, byte tcs, byte din, byte dout, byte irq

AxisPos arm1;
AxisPos arm2;

// Global LCD theme color variables
#define themeBackground 0xFFFF // White
#define menuBtnText 0xFFFF // White
#define menuBtnBorder 0x0000 // Black
#define menuBtnColor 0xFC00 // Orange
#define menuBackground 0xC618 //Silver

// For the draw shape functions
#define LEFT 1
#define CENTER 2
#define RIGHT 3

// Arm 1 IDs
#define ARM1_RX 0x0CA;
#define ARM1_T 0xA2
#define ARM1_B 0xA1
#define ARM1_CONTROL 0x0A0

// Arm 1 IDs
#define ARM1_RX 0x0CB;
#define ARM1_T 0xB2
#define ARM1_B 0xB1
#define ARM1_CONTROL 0x0B0

// Prevents physical button doubletap
#define BUTTON_DELAY 200

#define CSPIN 4   //slecting DPin-4 does not require to set direction
File myFile;      //file pointer variavle declaration

// DELETE ME
byte test = 0x07;

// For touch controls
int x, y;

// External import for fonts
// extern uint8_t SmallFont[];
extern uint8_t BigFont[];

CANBus can1;

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

// Draw the manual control page
void drawManualControl()
{

    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);
    drawSquareBtn(180, 10, 400, 45, "Manual Control", themeBackground, themeBackground, menuBtnColor, CENTER);
    drawSquareBtn(165, 235, 220, 275, "Arm", themeBackground, themeBackground, menuBtnColor, CENTER);
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
    drawSquareBtn(146, 260, 200, 315, "1", menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
    drawSquareBtn(201, 260, 255, 315, "2", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

return;
}

void manualControlBtns()
{
    int multiply = 1; // MOVE ME or something
    unsigned int txId = 0x0A3;
    uint8_t reverse = 0x10;
    byte data[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    while (true)
    {
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

            if ((y >= 110) && (y <= 169))
            {
                // A1 Up
                // 30, 110, 90, 170
                if ((x >= 30) && (x <= 90))
                {
                    data[1] = 1 * multiply;
                    waitForIt(30, 110, 90, 170);
                    data[1] = 0;
                }
                // A2 Up
                // 90, 110, 150, 170
                if ((x >= 90) && (x <= 150))
                {
                    data[2] = 1 * multiply;
                    waitForIt(90, 110, 150, 170);
                    data[2] = 0;
                }
                // A3 Up
                // 150, 110, 210, 170
                if ((x >= 150) && (x <= 210))
                {
                    data[3] = 1 * multiply;
                    waitForIt(150, 110, 210, 170);
                    data[3] = 0;
                }
                // A4 Up
                // 210, 110, 270, 170
                if ((x >= 210) && (x <= 270))
                {
                    data[4] = 1 * multiply;
                    waitForIt(210, 110, 270, 170);
                    data[4] = 0;
                }
                // A5 Up
                // 270, 110, 330, 170
                if ((x >= 270) && (x <= 330))
                {
                    data[5] = 1 * multiply;
                    waitForIt(270, 110, 330, 170);
                    data[5] = 0;
                }
                // A6 Up
                // 330, 110, 390, 170
                if ((x >= 330) && (x <= 390))
                {
                    data[6] = 1 * multiply;
                    waitForIt(330, 110, 390, 170);
                    data[6] = 0;
                }
                // A7 Up
                // 390, 110, 450, 170
                if ((x >= 390) && (x <= 450))
                {
                    data[7] = 1 * multiply;
                    waitForIt(390, 110, 450, 170);
                    data[7] = 0;
                }
            }
            // A1 Down
            // 30, 170, 90, 230
            if ((y >= 170) && (y <= 230))
            {
                if ((x >= 30) && (x <= 90))
                {
                    data[1] = (1 * multiply) + reverse;
                    waitForIt(30, 170, 90, 230);
                    data[1] = 0;
                }
                // A2 Down
                // 90, 170, 150, 230
                if ((x >= 90) && (x <= 150))
                {
                    data[2] = (1 * multiply) + reverse;
                    waitForIt(90, 170, 150, 230);
                    data[2] = 0;
                }
                // A3 Down
                // 150, 170, 210, 230
                if ((x >= 150) && (x <= 210))
                {
                    data[3] = (1 * multiply) + reverse;
                    waitForIt(150, 170, 210, 230);
                    data[3] = 0;
                }
                // A4 Down
                // 210, 170, 270, 230
                if ((x >= 210) && (x <= 270))
                {
                    data[4] = (1 * multiply) + reverse;
                    waitForIt(210, 170, 270, 230);
                    data[4] = 0;
                }
                // A5 Down
                // 270, 170, 330, 230
                if ((x >= 270) && (x <= 330))
                {
                    data[5] = (1 * multiply) + reverse;
                    waitForIt(270, 170, 330, 230);
                    data[5] = 0;
                }
                // A6 Down
                // 330, 170, 390, 230
                if ((x >= 330) && (x <= 390))
                {
                    data[6] = (1 * multiply) + reverse;
                    waitForIt(330, 170, 390, 230);
                    data[6] = 0;
                }
                // A7 Down
                // 390, 170, 450, 230
                if ((x >= 390) && (x <= 450))
                {
                    data[7] = (1 * multiply) + reverse;
                    waitForIt(390, 170, 450, 230);
                    data[7] = 0;
                }
            }

            if ((y >= 275) && (y <= 315))  // Upper row
            {
                if ((x >= 125) && (x <= 175))  // Button: 1
                {
                    // Select arm
                    //drawRoundButton(75, 275, 125, 315, "1", themeColor);
                    //drawRoundButton(125, 275, 175, 315, "2", themeSelected);
                    txId = 0x0B3;
                }
                if ((x >= 75) && (x <= 125))  // Button: 1
                {
                    // Select arm
                    //drawRoundButton(75, 275, 125, 315, "1", themeSelected);
                    //drawRoundButton(125, 275, 175, 315, "2", themeColor);
                    txId = 0x0A3;
                }
                // Back Button
                if ((x >= 365) && (x <= 475))
                {
                    waitForIt(365, 275, 475, 315);
                    return;
                }
            }
        }
    }
}

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
}

void drawProgram()
{
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);

    drawRoundBtn(180, 60, 360, 100, "Axis Test", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

void drawConfig()
{
    drawSquareBtn(141, 1, 478, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
    print_icon(435, 5, robotarm);

    drawRoundBtn(180, 60, 360, 100, "Settings", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

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

    can1.sendData(test2, 0x49);
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
                arm1.updateAxisPos(myGLCD);
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
            break;
        case 3:
            if (!hasDrawn)
            {
                drawManualControl();
                hasDrawn = true;
            }
            // Call buttons if any
            break;
        case 4:
            if (!hasDrawn)
            {
                drawConfig();
                hasDrawn = true;
            }
            // Call buttons if any
            break;
        case 5:
            if (!hasDrawn)
            {
                
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