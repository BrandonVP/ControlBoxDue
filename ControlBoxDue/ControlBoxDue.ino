/*
 Name:    ControlBoxDue.ino
 Created: 11/15/2020 8:27:18 AM
 Author:  Brandon Van Pelt
*/

/*=========================================================
	TODO List
===========================================================
Assign physical buttons
Switch between LI9486 & LI9488
Swtich between Due & mega2560

- Use Debug for diagnostic messages

- Add execute confirmation timeout
===========================================================
	End TODO List
=========================================================*/


#include <DS3231.h>
#include <LinkedList.h>
#include <malloc.h>
#include <SPI.h>

#include "Variables.h"
#include "KeyInput.h"
#include "definitions.h"
#include "CANBusWiFi.h"
#include "Program.h"
#include "icons.h"
#include "Common.h"
#include "KeyInput.h"

/*=========================================================
	Settings
===========================================================*/
// Select display
#define LI9488
#define DEBUG_KEYBOARD

#if defined LI9486
#include <TouchScreen.h>
#include <MCUFRIEND_kbv.h>
#include <UTFTGLUE.h>

#define MINPRESSURE 200
#define MAXPRESSURE 1000

int16_t BOXSIZE;
int16_t PENRADIUS = 1;
uint16_t ID, oldcolor, currentcolor;
uint8_t Orientation = 0;    //PORTRAIT

// Initialize display
//(byte model, int RS, int WR, int CS, int RST, int SER)
UTFTGLUE myGLCD(0, A2, A1, A3, A4, A0); //all dummy args

char* name = "Please Calibrate.";  //edit name of shield
const int XP = 6, XM = A2, YP = A1, YM = 7; //ID=0x9341
const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;
#elif defined LI9488
#include <memorysaver.h>
#include <UTouchCD.h>
#include <UTouch.h>
UTFT myGLCD(ILI9488_16, 7, 38, 9, 10);
//RTP: byte tclk, byte tcs, byte din, byte dout, byte irq
UTouch  myTouch(2, 6, 3, 4, 5);
// External import for fonts
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
#endif // 

// For touch controls
int x, y;

// Object to control RTC hardware
DS3231 rtc(SDA, SCL);

// Object to control CAN Bus hardware
CANBus can1;

// Object to control SD Card Hardware
SDCard sdCard;

// AxisPos object used to get current angles of robot arm
AxisPos axisPos;

// Linked list of nodes for a program
LinkedList<Program*> runList = LinkedList<Program*>();

//TODO: Clean these up
// Keeps track of current page
uint8_t controlPage = 1;
uint8_t page = 1;
//uint8_t nextPage = 1;
uint8_t oldPage = 1;
bool hasDrawn = false;
uint8_t graphicLoaderState = 0;
uint8_t errorMessageReturn = 2;
uint8_t state = 0;

// Variable to track current scroll position
uint8_t programScroll = 0;
uint16_t scroll = 0;

uint16_t waitTime = 0;
// CAN message ID and frame, value can be changed in manualControlButtons
uint16_t txIdManual = ARM1_MANUAL;

// Execute variables
bool programLoaded = false;
bool programRunning = false;
bool loopProgram = true;
bool Arm1Ready = true;
bool Arm2Ready = true;

// Replace with programState
bool programOpen = false;
bool programEdit = false;

uint8_t programProgress = 0; // THIS WILL LIMIT THE SIZE OF A PROGRAM TO 255 MOVEMENTS
uint8_t selectedProgram = 0;


const PROGMEM uint32_t hexTable[8] = { 1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 };


// 0 = open, 1 = close, 2 = no change
int8_t gripStatus = 2;

// Timer used for touch button press
uint32_t timer = 0;
// Timer used for RTC
uint32_t updateClock = 0;

// Key input variables
//char keyboardInput[9];
//uint8_t keypadInput[4] = { 0, 0, 0, 0 };
uint8_t keyIndex = 0;
uint8_t keyResult = 0;
uint16_t totalValue = 0;

char fileList[MAX_PROGRAMS][8];
uint8_t programCount = 0;

/*
Uncomment to update the clock then comment out and upload to
the device a second time to prevent updating time to last time
device was on at every startup.
*/
//#define UPDATE_CLOCK

// https://forum.arduino.cc/t/due-software-reset/332764/5
//Defines so the device can do a self reset
#define SYSRESETREQ    (1<<2)
#define VECTKEY        (0x05fa0000UL)
#define VECTKEY_MASK   (0x0000ffffUL)
#define AIRCR          (*(uint32_t*)0xe000ed0cUL) // fixed arch-defined address
#define REQUEST_EXTERNAL_RESET (AIRCR=(AIRCR&VECTKEY_MASK)|VECTKEY|SYSRESETREQ)

/*=========================================================
	Framework Functions
===========================================================*/
// Found this code somewhere online to draw BMP files
void bmpDraw(char* filename, int x, int y) {
	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * PIXEL_BUFFER]; // pixel in buffer (R+G+B per pixel)
	uint16_t lcdbuffer[PIXEL_BUFFER];  // pixel out buffer (16-bit per pixel)
	uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      w, h, row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();
	uint8_t  lcdidx = 0;
	boolean  first = true;
	int dispx = myGLCD.getDisplayXSize();
	int dispy = myGLCD.getDisplayYSize();

	if ((x >= dispx) || (y >= dispy)) return;

	Serial.println();
	Serial.print(F("Loading image '"));
	Serial.print(filename);
	Serial.println('\'');

	// Open requested file on SD card
	if ((bmpFile = SD.open(filename)) == NULL) {
		Serial.println(F("File not found"));
		return;
	}


	// Parse BMP header
	if (read16(bmpFile) == 0x4D42) { // BMP signature

		Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: "));
		Serial.println(bmpImageoffset, DEC);

		// Read DIB header
		Serial.print(F("Header size: "));
		Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);

		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print(F("Bit Depth: "));
			Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print(F("Image size: "));
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				w = bmpWidth;
				h = bmpHeight;
				if ((x + w - 1) >= dispx)  w = dispx - x;
				if ((y + h - 1) >= dispy) h = dispy - y;

				// Set TFT address window to clipped image bounds
				for (row = 0; row < h; row++) { // For each scanline...
					// Seek to start of scan line.  It might seem labor-
					// intensive to be doing this on every line, but this
					// method covers a lot of gritty details like cropping
					// and scanline padding.  Also, the seek only takes
					// place if the file position actually needs to change
					// (avoids a lot of cluster math in SD library).
					if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if (bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}
					for (col = 0; col < w; col++) { // For each column...
						// Time to read more pixel data?
						if (buffidx >= sizeof(sdbuffer)) { // Indeed
							// Push LCD buffer to the display first
							if (lcdidx > 0) {
								myGLCD.setColor(lcdbuffer[lcdidx]);
								myGLCD.drawPixel(col + 117, row + 1);
								lcdidx = 0;
								first = false;
							}
							bmpFile.read(sdbuffer, sizeof(sdbuffer));
							buffidx = 0; // Set index to beginning
						}

						// Convert pixel from BMP to TFT format
						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						myGLCD.setColor(r, g, b);
						myGLCD.drawPixel(col + 117, row + 1);

					} // end pixel
				} // end scanline

				// Write any remaining data to LCD
				if (lcdidx > 0) {
					myGLCD.setColor(lcdbuffer[lcdidx]);
					myGLCD.drawPixel(col + 117, row + 1);
				}
			} // end goodBmp
		}
	}

	bmpFile.close();
	if (!goodBmp) Serial.println(F("BMP format not recognized."));

}
// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t read16(File f) {
	uint16_t result;
	((uint8_t*)&result)[0] = f.read(); // LSB
	((uint8_t*)&result)[1] = f.read(); // MSB
	return result;
}
uint32_t read32(File f) {
	uint32_t result;
	((uint8_t*)&result)[0] = f.read(); // LSB
	((uint8_t*)&result)[1] = f.read();
	((uint8_t*)&result)[2] = f.read();
	((uint8_t*)&result)[3] = f.read(); // MSB
	return result;
}
// End BMP Draw

// I converted a google robot arm image to a bitmap using Sib Icon Studio
// Then wrote this function to print the bits (located in icons.h)
void print_icon(uint16_t x, uint16_t y, const unsigned char icon[])
{
	myGLCD.setColor(menuBtnColor);
	myGLCD.setBackColor(themeBackground);
	int16_t i = 0, row, column, bit, temp;
	for (row = 0; row < 40; row++)
	{
		for (column = 0; column < 5; column++)
		{
			temp = icon[i];
			for (bit = 7; bit >= 0; bit--)
			{
				if (temp & 1) { myGLCD.drawPixel(x + (column * 8) + (8 - bit), y + row); }
				temp >>= 1;
			}
			i++;
		}
	}
}

// Function to fine memory use
// https://forum.arduino.cc/t/getting-heap-size-stack-size-and-free-ram-from-due/678195/5
extern char _end;
extern "C" char* sbrk(int i);
void saveRamStates(uint32_t MaxUsedHeapRAM, uint32_t MaxUsedStackRAM, uint32_t MaxUsedStaticRAM, uint32_t MinfreeRAM)
{
	char* ramstart = (char*)0x20070000;
	char* ramend = (char*)0x20088000;

	char* heapend = sbrk(0);
	register char* stack_ptr asm("sp");
	struct mallinfo mi = mallinfo();
	if (MaxUsedStaticRAM < &_end - ramstart)
	{
		MaxUsedStaticRAM = &_end - ramstart;
	}
	if (MaxUsedHeapRAM < mi.uordblks)
	{
		MaxUsedHeapRAM = mi.uordblks;
	}
	if (MaxUsedStackRAM < ramend - stack_ptr)
	{
		MaxUsedStackRAM = ramend - stack_ptr;
	}
	if (MinfreeRAM > stack_ptr - heapend + mi.fordblks || MinfreeRAM == 0)
	{
		MinfreeRAM = stack_ptr - heapend + mi.fordblks;
	}

	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	drawRoundBtn(135, 80, 310, 130, F("Used RAM"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 80, 475, 130, String(MaxUsedStaticRAM), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(135, 135, 310, 185, F("Used HEAP"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 135, 475, 185, String(MaxUsedHeapRAM), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(135, 190, 310, 240, F("Used STACK"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 190, 475, 240, String(MaxUsedStackRAM), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(135, 245, 310, 295, F("FREE RAM"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 245, 475, 295, String(MinfreeRAM), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}
void memoryUse()
{
	uint32_t MaxUsedHeapRAM = 0;
	uint32_t MaxUsedStackRAM = 0;
	uint32_t MaxUsedStaticRAM = 0;
	uint32_t MinfreeRAM = 0;
	saveRamStates(MaxUsedHeapRAM, MaxUsedStackRAM, MaxUsedStaticRAM, MinfreeRAM);
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
		SerialUSB.println("d");
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
	uint32_t timer = millis();
	while (Touch_getXY() || millis() - timer < 20)
	{
		if (Touch_getXY())
		{
			timer = millis();
		}
		backgroundProcess();
	}
	myGLCD.setColor(menuBtnBorder);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}

// Highlights square buttons when selected
void waitForItRect(int x1, int y1, int x2, int y2)
{
	myGLCD.setColor(themeBackground);
	myGLCD.drawRect(x1, y1, x2, y2);
	unsigned long timer = millis();
	while (Touch_getXY() || millis() - timer < 20)
	{
		if (Touch_getXY())
		{
			timer = millis();
		}
		backgroundProcess();
	}
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
		backgroundProcess();
		axisPos.drawAxisPosUpdateM(myGLCD, txIdManual, false);
		// TODO replace delay with something non blocking
		delay(40);
		axisPos.drawAxisPosUpdateM(myGLCD, txIdManual, false);
		delay(40);
	}
	myGLCD.setColor(menuBtnBorder);
	myGLCD.drawRect(x1, y1, x2, y2);
}

bool Touch_getXY()
{
#if defined LI9486
	TSPoint p = ts.getPoint();
	pinMode(YP, OUTPUT);      //restore shared pins
	pinMode(XM, OUTPUT);
	digitalWrite(YP, HIGH);   //because TFT control pins
	digitalWrite(XM, HIGH);
	bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
	if (pressed)
	{
		y = 320 - map(p.x, TS_LEFT, TS_RT, 0, 320); //.kbv makes sense to me
		x = map(p.y, TS_TOP, TS_BOT, 0, 480);
	}
	return pressed;
#elif defined LI9488
	if (myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();
		return true;
	}
	return false;
#endif
}


/*=========================================================
	Manual control Functions
===========================================================*/
// Draw the manual control page
bool drawManualControl()
{
	// Variables for loop iterations within the switch statement
	uint8_t j;
	uint16_t i;
	switch (graphicLoaderState)
	{
	case 0:
		// Clear LCD to be written 
		drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
		break;
	case 1:
		// Print arm logo
		print_icon(435, 5, robotarm);
		break;
	case 2:
		// Print page title
		drawSquareBtn(180, 10, 400, 45, F("Manual Control"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 3:
		// Manual control axis labels
		j = 1;
		for (i = 131; i < (480 - 45); i = i + 58) {
			myGLCD.setColor(menuBtnColor);
			myGLCD.setBackColor(themeBackground);
			myGLCD.printNumI(j, i + 20, 50);
			j++;
		}
		break;
	case 4:
		// Draw the upper row of movement buttons
		for (i = 131; i < (480 - 54); i = i + 58) {
			drawSquareBtn(i, 70, i + 54, 125, F("/\\"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		break;
	case 5:
		// Draw the upper row of movement buttons
		for (i = 131; i < (480 - 54); i = i + 58) {
			drawSquareBtn(i, 125, i + 54, 175, "", menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		break;
	case 6:
		// Draw the bottom row of movement buttons
		for (i = 131; i < (480 - 54); i = i + 58) {
			drawSquareBtn(i, 175, i + 54, 230, F("\\/"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		break;
	case 7:
		// Draw Select arm buttons
		drawSquareBtn(131, 249, 231, 259, F("Arm"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 8:
		if (txIdManual == ARM1_MANUAL)
		{
			drawSquareBtn(131, 270, 181, 319, F("1"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
			drawSquareBtn(181, 270, 231, 319, F("2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		else if (txIdManual == ARM2_MANUAL)
		{
			drawSquareBtn(131, 270, 181, 319, F("1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
			drawSquareBtn(181, 270, 231, 319, F("2"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
		}
		break;
	case 9:
		// Draw grip buttons
		drawSquareBtn(305, 251, 475, 269, F("Gripper"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 10:
		drawSquareBtn(305, 270, 390, 319, F("Open"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 11:
		drawSquareBtn(390, 270, 475, 319, F("Close"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 12:
		return true;
		break;
	}
	return false;
}

// Draw page button function
void manualControlButtons()
{
	// Mutiply is for a future funtion to allow movement of multiple degrees per button press instead of 1
	uint8_t multiply = 1;

	// Enables revese
	uint8_t reverse = 0x10;

	// CAN Bus message data
	byte data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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
	if (Touch_getXY())
	{
		if ((y >= 70) && (y <= 126))
		{
			// A1 Up
			if ((x >= 131) && (x <= 185))
			{
				data[1] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(131, 70, 185, 126, txIdManual, data);
				data[1] = 0;
			}
			// A2 Up
			if ((x >= 189) && (x <= 243))
			{
				data[2] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(189, 70, 243, 126, txIdManual, data);
				data[2] = 0;
			}
			// A3 Up
			if ((x >= 247) && (x <= 301))
			{
				data[3] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(247, 70, 301, 126, txIdManual, data);
				data[3] = 0;
			}
			// A4 Up
			if ((x >= 305) && (x <= 359))
			{
				data[4] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(305, 70, 359, 126, txIdManual, data);
				data[4] = 0;
			}
			// A5 Up
			if ((x >= 363) && (x <= 417))
			{
				data[5] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(363, 70, 417, 126, txIdManual, data);
				data[5] = 0;
			}
			// A6 Up
			if ((x >= 421) && (x <= 475))
			{
				data[6] = 1 * multiply;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(421, 70, 475, 126, txIdManual, data);
				data[6] = 0;
			}
		}
		if ((y >= 125) && (y <= 175))
		{
			// A1 Deg
			if ((x >= 131) && (x <= 185))
			{
				waitForItRect(131, 125, 185, 175);

			}
			// A2 Deg
			if ((x >= 189) && (x <= 243))
			{
				waitForItRect(189, 125, 243, 175);

			}
			// A3 Deg
			if ((x >= 247) && (x <= 301))
			{
				waitForItRect(247, 125, 301, 175);

			}
			// A4 Deg
			if ((x >= 305) && (x <= 359))
			{
				waitForItRect(305, 125, 359, 175);

			}
			// A5 Deg
			if ((x >= 363) && (x <= 417))
			{
				waitForItRect(363, 125, 417, 175);

			}
			// A6 Deg
			if ((x >= 421) && (x <= 475))
			{
				waitForItRect(421, 125, 475, 175);
				
			}
		}
		if ((y >= 175) && (y <= 230))
		{
			// A1 Down
			if ((x >= 131) && (x <= 185))
			{
				data[1] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(131, 175, 185, 230, txIdManual, data);
				data[1] = 0;
			}
			// A2 Down
			if ((x >= 189) && (x <= 243))
			{
				data[2] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(189, 175, 243, 230, txIdManual, data);
				data[2] = 0;
			}
			// A3 Down
			if ((x >= 247) && (x <= 301))
			{
				data[3] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(247, 175, 301, 230, txIdManual, data);
				data[3] = 0;
			}
			// A4 Down
			if ((x >= 305) && (x <= 359))
			{
				data[4] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(305, 175, 359, 230, txIdManual, data);
				data[4] = 0;
			}
			// A5 Down
			if ((x >= 363) && (x <= 417))
			{
				data[5] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(363, 175, 417, 230, txIdManual, data);
				data[5] = 0;
			}
			// A6 Down
			if ((x >= 421) && (x <= 475))
			{
				data[6] = (1 * multiply) + reverse;
				data[CRC_BYTE] = generateCRC(data, 7);
				waitForItRect(421, 175, 475, 230, txIdManual, data);
				data[6] = 0;
			}
		}
		if ((y >= 270) && (y <= 319))
		{
			if ((x >= 146) && (x <= 200))
			{
				// Select arm 1
				drawSquareBtn(131, 270, 181, 319, F("1"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
				drawSquareBtn(181, 270, 231, 319, F("2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				txIdManual = ARM1_MANUAL;
			}
			if ((x >= 200) && (x <= 254))
			{
				// Select arm 2
				drawSquareBtn(131, 270, 181, 319, F("1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				drawSquareBtn(181, 270, 231, 319, F("2"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
				txIdManual = ARM2_MANUAL;
			}
			if ((x >= 305) && (x <= 390))
			{
				// Grip open
				data[7] = 1 * multiply;
				waitForItRect(305, 270, 390, 319, txIdManual, data);
				data[7] = 0;
			}
			if ((x >= 390) && (x <= 475))
			{
				// Grip close
				data[7] = (1 * multiply) + reverse;
				waitForItRect(390, 270, 475, 319, txIdManual, data);
				data[7] = 0;
			}
		}
	}
}

/*=========================================================
					View page
===========================================================*/
// Draw the view page
bool drawView()
{
	// Variable for loop in switch statement
	uint16_t start, stop, row;

	// Boxes for current arm angles
	const PROGMEM uint16_t xStart = 190;
	const PROGMEM uint16_t xStop = 295;
	const PROGMEM uint8_t yStart = 35;
	const PROGMEM uint8_t yStop = 75;

	switch (graphicLoaderState)
	{
	case 0:
		// Clear LCD to be written 
		drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
		break;
	case 1:
		// Print arm logo
		print_icon(435, 5, robotarm);
		break;
	case 2:
		// Draw row lables
		for (start = 35, stop = 75, row = 1; start <= 260; start = start + 45, stop = stop + 45, row++)
		{
			String rowLable = "A" + String(row);
			drawRoundBtn(160, start, 180, stop, rowLable, themeBackground, themeBackground, menuBtnColor, CENTER);
		}
		break;
	case 3:
		// Arm 1
		drawRoundBtn(xStart + 110, 5, xStop + 110, 40, F("Arm2"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 4:
		drawRoundBtn(xStart + 110, yStart + 0, xStop + 110, yStop + 0, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 5:
		drawRoundBtn(xStart + 110, yStart + 45, xStop + 110, yStop + 45, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 6:
		drawRoundBtn(xStart + 110, yStart + 90, xStop + 110, yStop + 90, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 7:
		drawRoundBtn(xStart + 110, yStart + 135, xStop + 110, yStop + 135, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 8:
		drawRoundBtn(xStart + 110, yStart + 180, xStop + 110, yStop + 180, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 9:
		drawRoundBtn(xStart + 110, yStart + 225, xStop + 110, yStop + 225, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 10:
		// Arm 2
		drawRoundBtn(xStart, 5, xStop, 40, F("Arm1"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 11:
		drawRoundBtn(xStart, yStart + 0, xStop, yStop + 0, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 12:
		drawRoundBtn(xStart, yStart + 45, xStop, yStop + 45, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 13:
		drawRoundBtn(xStart, yStart + 90, xStop, yStop + 90, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 14:
		drawRoundBtn(xStart, yStart + 135, xStop, yStop + 135, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 15:
		drawRoundBtn(xStart, yStart + 180, xStop, yStop + 180, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 16:
		drawRoundBtn(xStart, yStart + 225, xStop, yStop + 225, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
		break;
	case 17:
		return true;
		break;
	}
	graphicLoaderState++;
	return false;
}


/*==========================================================
					Program Arm
============================================================*/
// Main program state
void program()
{
	static uint8_t programState = 0;
	switch (programState)
	{
	case 0:
		
		break;
	case 1:
		break;
	case 2:
		break;
	}
}

// Draws scrollable box that contains 10 slots for programs
void drawProgramScroll()
{
	File dirList;
	dirList = SD.open("/PROGRAMS/");

	programCount = sdCard.printDirectory(dirList, fileList);

	// selected position = programScroll * position
	// if selected draw different color border
	uint16_t yAxis = 55;
	const PROGMEM uint16_t xAxis = 133;
	for (int i = 0; i < 5; i++)
	{
		//if (fileList[i + programScroll].compareTo("") != 0 && sdCard.fileExists(fileList[i + programScroll]))
		if (i + programScroll < programCount)
		{
			drawSquareBtn(xAxis, yAxis, 284, yAxis + 40, (fileList[i + programScroll]), menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
			drawSquareBtn(284, yAxis, 351, yAxis + 40, F("LOAD"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
			drawSquareBtn(351, yAxis, 420, yAxis + 40, F("EDIT"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
			drawSquareBtn(420, yAxis, 477, yAxis + 40, F("DEL"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		else
		{
			//drawSquareBtn(xAxis, yAxis, 410, yAxis + 40, (aList[i + programScroll] + "-Empty"), menuBackground, menuBtnBorder, menuBtnText, LEFT);
			drawSquareBtn(xAxis, yAxis, 478, yAxis + 40, "", menuBackground, menuBtnBorder, menuBtnText, LEFT);
		}

		yAxis += 45;
	}
}

// Draws buttons for program function
bool drawProgram()
{
	switch (graphicLoaderState)
	{
	case 0:
		break;
	case 1:
		// Clear LCD to be written
		drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
		break;
	case 2:
		// Print arm logo
		print_icon(435, 5, robotarm);
		break;
	case 3:
		// Print page title
		drawSquareBtn(180, 10, 400, 45, F("Program"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 4:
		drawSquareBtn(132, 54, 478, 96, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 5:
		drawSquareBtn(132, 99, 478, 141, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 6:
		drawSquareBtn(132, 144, 478, 186, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 7:
		drawSquareBtn(132, 189, 478, 231, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 8:
		drawSquareBtn(132, 234, 478, 276, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 9:
		drawSquareBtn(132, 279, 211, 318, F(""), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 10:
		drawSquareBtn(133, 280, 210, 317, F("/\\"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 11:
		drawSquareBtn(260, 279, 351, 318, F(""), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 12:
		drawSquareBtn(261, 280, 350, 317, F("Add"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 13:
		drawSquareBtn(400, 279, 478, 318, F(""), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 14:
		drawSquareBtn(401, 280, 477, 317, F("\\/"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 15:
		drawProgramScroll();
		return true;
		break;
	}
	graphicLoaderState++;
	return false;
}

// Deletes current selected program from 
void programDelete()
{
	char programDirectory[20] = "PROGRAMS/";
	strcat(programDirectory, fileList[selectedProgram]);
	sdCard.deleteFile(programDirectory);
}

// Load selected program from SD card into linked list
void loadProgram()
{
	char programDirectory[20] = "PROGRAMS/";
	strcat(programDirectory, fileList[selectedProgram]);
	sdCard.readFile(programDirectory, runList);
}

// Button functions for program page 
void programButtons()
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((y >= 55) && (y <= 95) && programCount > 0 + programScroll)
		{
			selectedProgram = 0 + programScroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 55, 351, 95);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
				drawExecuteButton();
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 55, 420, 95);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				graphicLoaderState = 0;
				page = 8;
				hasDrawn = false;
				drawExecuteButton();
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 55, 477, 95);
				// DEL
				drawErrorMSG(F("Confirmation"), F("Delete"), fileList[selectedProgram]);
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 100) && (y <= 140) && programCount > 1 + programScroll)
		{
			selectedProgram = 1 + programScroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 100, 351, 140);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
				drawExecuteButton();
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 100, 420, 140);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				graphicLoaderState = 0;
				page = 8;
				hasDrawn = false;
				drawExecuteButton();
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 100, 477, 140);
				// DEL
				drawErrorMSG(F("Confirmation"), F("Delete"), fileList[selectedProgram]);
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 145) && (y <= 185) && programCount > 2 + programScroll)
		{
			selectedProgram = 2 + programScroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 145, 351, 185);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
				drawExecuteButton();
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 145, 420, 185);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				graphicLoaderState = 0;
				page = 8;
				hasDrawn = false;
				drawExecuteButton();
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 145, 477, 185);
				// DEL
				drawErrorMSG(F("Confirmation"), F("Delete"), fileList[selectedProgram]);
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 190) && (y <= 230) && programCount > 3 + programScroll)
		{
			selectedProgram = 3 + programScroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 190, 351, 230);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
				drawExecuteButton();
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 190, 420, 230);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				graphicLoaderState = 0;
				page = 8;
				hasDrawn = false;
				drawExecuteButton();
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 190, 477, 230);
				// DEL
				drawErrorMSG(F("Confirmation"), F("Delete"), fileList[selectedProgram]);
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 235) && (y <= 275) && programCount > 4 + programScroll)
		{
			selectedProgram = 4 + programScroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 235, 351, 275);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
				drawExecuteButton();
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 235, 420, 275);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				graphicLoaderState = 0;
				page = 8;
				hasDrawn = false;
				drawExecuteButton();
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 235, 477, 275);
				// DEL
				drawErrorMSG(F("Confirmation"), F("Delete"), fileList[selectedProgram]);
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 280) && (y <= 317))
		{
			if ((x >= 133) && (x <= 210))
			{

				// Scroll up
				waitForIt(133, 280, 210, 317);
				if (programScroll > 0)
				{
					programScroll--;
					drawProgramScroll();
				}
			}
			if ((x >= 401) && (x <= 477))
			{
				// Scroll down
				waitForIt(401, 280, 477, 317);
				if (programScroll < 5)
				{
					programScroll++;
					drawProgramScroll();
				}
			}
			if ((x >= 261) && (x <= 350))
			{
				// Edit program
				waitForItRect(261, 280, 350, 317);
				// Add
				selectedProgram = findProgramNode(); // Find an empty program slot to open
				runList.clear(); // Empty the linked list of any program currently loaded
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
		}
	}
}

// TODO: Might be a bug if programCount is incremented but user cancels the program creation. Nothing observed when trying but need to investigate
uint8_t findProgramNode()
{
	if (programCount < MAX_PROGRAMS)
	{
		return programCount++;
	}
	else
	{
		return 0xFF;
	}
}

// Incase it needs to be moved/edited in the future
void drawExecuteButton()
{
	drawRoundBtn(4, 260, 122, 310, F("5-EXEC"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}


/*==========================================================
					Edit Program
============================================================*/
bool drawEditPage()
{
	switch (graphicLoaderState)
	{
	case 0:
		// Clear LCD to be written
		drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
		break;
	case 1:
		print_icon(435, 5, robotarm);
		break;
	case 2:
		drawSquareBtn(180, 10, 400, 45, F("Edit Program"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 3:
		drawSquareBtn(132, 54, 478, 96, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 4:
		drawSquareBtn(133, 55, 280, 95, F("Filename"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 5:
		drawSquareBtn(280, 55, 477, 95, fileList[selectedProgram], menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 6:
		drawSquareBtn(132, 99, 478, 141, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 7:
		drawSquareBtn(133, 100, 280, 140, F("Size"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 8:
		drawSquareBtn(280, 100, 477, 140, String(runList.size()) + "/255", menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 9:
		drawSquareBtn(132, 144, 478, 186, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 10:
		drawSquareBtn(132, 189, 478, 231, F("Edit Steps"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 11:
		drawSquareBtn(132, 234, 478, 276, F("Save & Exit"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 12:
		drawSquareBtn(132, 280, 478, 317, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 13:
		return true;
		break;
	}
	graphicLoaderState++;
	return false;
}

void editPageButtons()
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((x >= 280) && (x <= 477))
		{
			if ((y >= 55) && (y <= 95))
			{
				// Name File
				waitForItRect(280, 55, 477, 95);
				hasDrawn = false;
				page = 9;
			}
		}
		if ((x >= 132) && (x <= 478))
		{
			if ((y >= 189) && (y <= 231))
			{
				// Edit
				waitForItRect(132, 189, 478, 231);
				programEdit = true;
				page = 6;
				hasDrawn = false;
			}
			if ((y >= 234) && (y <= 276))
			{
				// Save program
				waitForItRect(132, 234, 478, 276);
				programDelete();
				saveProgram();
				programOpen = false;
				programLoaded = true;
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
				drawExecuteButton();
			}
			if ((y >= 280) && (y <= 317))
			{
				// Cancel
				waitForItRect(132, 280, 478, 317);
				programOpen = false;
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
			}
		}
	}
}

// Draws scrollable box that contains all the nodes in a program
void drawProgramEditScroll()
{
	myGLCD.setFont(SmallFont);
	uint8_t nodeSize = runList.size();

	// Each node should be listed with all information, might need small text
	uint16_t row = 5;
	for (uint8_t i = 0; i < 6; i++)
	{
		char gripString[5];
		switch (runList.get(i + scroll)->getGrip())
		{
		case 0:
			strcpy(gripString, "O");
			break;
		case 1:
			strcpy(gripString, "S");
			break;
		case 2:
			strcpy(gripString, "H");
			break;
		}

		char buffer[40];
		sprintf(buffer, "%3d|%3d|%3d|%3d|%3d|%3d|%3d|%s|%3X|%3d", (i + scroll + 1), runList.get(i + scroll)->getA1(), runList.get(i + scroll)->getA2(), runList.get(i + scroll)->getA3(), runList.get(i + scroll)->getA4(), runList.get(i + scroll)->getA5(), runList.get(i + scroll)->getA6(), gripString, runList.get(i + scroll)->getID(), runList.get(i + scroll)->getWait());

		(i + scroll < nodeSize) ? drawSquareBtn(130, row, 435, row + 37, buffer, menuBackground, menuBtnBorder, menuBtnText, LEFT) : drawSquareBtn(130, row, 435, row + 37, "", menuBackground, menuBtnBorder, menuBtnText, LEFT);

		row += 37;
	}
	// To edit a node just replace with a new position
	myGLCD.setFont(BigFont);
}

// Draws buttons for edit program function
void drawProgramEdit(uint8_t scroll = 0)
{
	// Clear LCD to be written
	drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

	// Scroll buttons
	myGLCD.setColor(menuBtnColor);
	myGLCD.setBackColor(themeBackground);
	drawSquareBtn(435, 5, 475, 116, F("/\\"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(435, 116, 475, 227, F("\\/"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	// Draw program edit buttons
	drawSquareBtn(130, 230, 215, 270, F("Add"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(215, 230, 300, 270, F("Ins"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(300, 230, 385, 270, F("Del"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(385, 230, 475, 270, F("Done"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(215, 275, 300, 315, F("Wait"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(300, 275, 385, 315, F("Sen"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(385, 275, 475, 315, F(""), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	switch (gripStatus)
	{
	case 0: drawSquareBtn(130, 275, 215, 315, F("Open"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 1: drawSquareBtn(130, 275, 215, 315, F("Close"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 2: drawSquareBtn(130, 275, 215, 315, F("Grip"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;

	}
}

// Adds current position to program linked list 
void addNode(int insert = -1)
{
	// Array of arm axis positions
	uint16_t posArray[8] = { 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000 };

	// Update array value with data collected from the axis position update
	if (txIdManual == ARM1_MANUAL)
	{
		posArray[0] = axisPos.getA1C1();
		posArray[1] = axisPos.getA2C1();
		posArray[2] = axisPos.getA3C1();
		posArray[3] = axisPos.getA4C1();
		posArray[4] = axisPos.getA5C1();
		posArray[5] = axisPos.getA6C1();

		// Create program object with array positions, grip on/off, and channel
		Program* node = new Program(posArray, gripStatus, waitTime, ARM1_PROGRAM);
		(insert < 0) ? runList.add(node) : runList.add(insert, node);
	}
	else if (txIdManual == ARM2_MANUAL)
	{
		posArray[0] = axisPos.getA1C2();
		posArray[1] = axisPos.getA2C2();
		posArray[2] = axisPos.getA3C2();
		posArray[3] = axisPos.getA4C2();
		posArray[4] = axisPos.getA5C2();
		posArray[5] = axisPos.getA6C2();
		
		// Create program object with array positions, grip on/off, and channel
		Program* node = new Program(posArray, gripStatus, waitTime, ARM2_PROGRAM);
		(insert < 0) ? runList.add(node) : runList.add(insert, node);
	}

	// Reset waitTime back to 0
	waitTime = 0;
}

// Delete node from linked list
void deleteNode(uint16_t nodeLocation)
{
	runList.remove(nodeLocation);
}

// Writes current selected linked list to SD card
void saveProgram()
{
	// Delimiter 
	String space = ", ";
	char programDirectory[20] = "PROGRAMS/";

	char buffer[8];
	sprintf(buffer, "%s", fileList[selectedProgram]);
	strcat(programDirectory, buffer);

	// Write out linkedlist data to text file
	for (uint8_t i = 0; i < runList.size(); i++)
	{
		sdCard.writeFile(programDirectory, ",");
		sdCard.writeFile(programDirectory, runList.get(i)->getA1());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getA2());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getA3());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getA4());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getA5());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getA6());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getID());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getWait());
		sdCard.writeFile(programDirectory, space);
		sdCard.writeFile(programDirectory, runList.get(i)->getGrip());
		sdCard.writeFileln(programDirectory);
	}
	if (runList.size() == 0)
	{
		sdCard.writeFileln(programDirectory);
	}
}

// Button functions for edit program page
void programEditButtons()
{
	static uint8_t selectedNode = 0;

	// Touch screen controls
	if (Touch_getXY())
	{
		if ((x >= 130) && (x <= 435))
		{
			if ((y >= 5) && (y <= 42))
			{
				waitForItRect(130, 5, 435, 42);
				//Serial.println(1 + scroll);
				selectedNode = 0 + scroll;
				drawProgramEditScroll();
			}
			if ((y >= 42) && (y <= 79))
			{
				waitForItRect(130, 42, 435, 79);
				//Serial.println(2 + scroll);
				selectedNode = 1 + scroll;
				drawProgramEditScroll();
			}
			if ((y >= 79) && (y <= 116))
			{
				waitForItRect(130, 79, 435, 116);
				//Serial.println(3 + scroll);
				selectedNode = 2 + scroll;
				drawProgramEditScroll();
			}
			if ((y >= 116) && (y <= 153))
			{
				waitForItRect(130, 116, 435, 153);
				//Serial.println(4 + scroll);
				selectedNode = 3 + scroll;
				drawProgramEditScroll();
			}
			if ((y >= 153) && (y <= 190))
			{
				waitForItRect(130, 153, 435, 190);
				//Serial.println(5 + scroll);
				selectedNode = 4 + scroll;
				drawProgramEditScroll();
			}
			if ((y >= 190) && (y <= 227))
			{
				waitForItRect(130, 190, 435, 227);
				//Serial.println(5 + scroll);
				selectedNode = 5 + scroll;
				drawProgramEditScroll();
			}
		}
		if ((x >= 436) && (x <= 475))
		{
			if ((y >= 5) && (y <= 116))
			{
				waitForIt(435, 5, 475, 116);
				if (scroll > 3)
				{
					scroll--;
					scroll--;
					scroll--;
					drawProgramEditScroll();
				}
				else
				{
					scroll = 0;
					drawProgramEditScroll();
				}
			}
			if ((y >= 116) && (y <= 227))
			{
				waitForIt(435, 116, 475, 227);
				if (scroll < runList.size())
				{
					if (scroll < 252)
					{
						scroll++;
						scroll++;
						scroll++;
						drawProgramEditScroll();
					}
					else
					{
						scroll = 255;
						drawProgramEditScroll();
					}
				}

			}
		}
		if ((y >= 230) && (y <= 270))
		{
			if ((x >= 130) && (x <= 215))
			{
				// Add node
				waitForItRect(130, 230, 215, 270);
				if (runList.size() < 255)
				{
					addNode();
					(runList.size() > 6) ? scroll++ : scroll = scroll;
					drawProgramEditScroll();
				}
			}
			if ((x >= 215) && (x <= 300))
			{
				// Insert node
				waitForItRect(215, 230, 300, 270);
				if (runList.size() < 255)
				{
					addNode(selectedNode);
					drawProgramEditScroll();
				}
			}
			if ((x >= 300) && (x <= 385))
			{
				// Delete node
				waitForItRect(300, 230, 385, 270);
				if (runList.size() > 0)
				{
					deleteNode(selectedNode);
					drawProgramEditScroll();
				}
			}
			if ((x >= 385) && (x <= 475))
			{
				// Save program
				waitForItRect(385, 230, 475, 270);
				programEdit = false;
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
				programLoaded = true;
			}
		}
		if ((y >= 275) && (y <= 315))
		{
			if ((x >= 215) && (x <= 300))
			{
				// wait
				waitForItRect(215, 275, 300, 315);
				hasDrawn = false;
				graphicLoaderState = 0;
				page = 10;
			}
			if ((x >= 300) && (x <= 385))
			{
				// 
				waitForItRect(300, 275, 385, 315);

			}
			if ((x >= 385) && (x <= 475))
			{
				// 
				waitForItRect(385, 275, 475, 315);
		
			}
			if ((x >= 130) && (x <= 215))
			{
				// Grip
				waitForItRect(130, 275, 215, 315);
				if (gripStatus < 2)
				{
					gripStatus++;
				}
				else
				{
					gripStatus = 0;
				}
				switch (gripStatus)
				{
				case 0:
					drawSquareBtn(130, 275, 215, 315, "Open", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				case 1:
					drawSquareBtn(130, 275, 215, 315, "Close", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				case 2:
					drawSquareBtn(130, 275, 215, 315, "Grip", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				}
			}
		}
	}
}


/*==========================================================
					Configure Arm
============================================================*/
// Draws the config page
bool drawConfig()
{
	switch (graphicLoaderState)
	{
	case 0:
		drawSquareBtn(X_PAGE_START, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
		break;
	case 1:
		print_icon(435, 5, robotarm);
		break;
	case 2:
		drawSquareBtn(180, 10, 400, 45, F("Configuration"), themeBackground, themeBackground, menuBtnColor, CENTER);
		break;
	case 3:
		drawRoundBtn(150, 60, 300, 100, F("Home Ch1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 4:
		drawRoundBtn(310, 60, 460, 100, F("Set Ch1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 5:
		drawRoundBtn(150, 110, 300, 150, F("Home Ch2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 6:
		drawRoundBtn(310, 110, 460, 150, F("Set Ch2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 7:
		drawRoundBtn(150, 160, 300, 200, F("Loop On"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 8:
		drawRoundBtn(310, 160, 460, 200, F("Loop Off"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 9:
		drawRoundBtn(150, 210, 300, 250, F("Memory"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 10:
		drawRoundBtn(310, 210, 460, 250, F("Reset"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 11:
		drawRoundBtn(150, 260, 300, 300, F("About"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 12:
		drawSquareBtn(150, 301, 479, 319, version, themeBackground, themeBackground, menuBtnColor, CENTER);
		return true;
		break;
	}
	graphicLoaderState++;
	return false;
}

// Sends command to return arm to starting position
void homeArm(uint16_t arm_control)
{
	byte cmd[8] = { 0x00,HOME_AXIS_POSITION, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	cmd[CRC_BYTE] = generateCRC(cmd, 7);
	can1.sendFrame(arm_control, cmd);
}

// Button functions for config page
void configButtons()
{
	uint8_t setHomeID[8] = { 0x00, RESET_AXIS_POSITION, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	setHomeID[CRC_BYTE] = generateCRC(setHomeID, 7);

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
				homeArm(ARM1_CONTROL);
			}
			if ((x >= 310) && (x <= 460))
			{
				waitForIt(310, 60, 460, 100);
				can1.sendFrame(ARM1_CONTROL, setHomeID);
			}
		}
		if ((y >= 110) && (y <= 150))
		{
			if ((x >= 150) && (x <= 300))
			{
				waitForIt(150, 110, 300, 150);
				homeArm(ARM2_CONTROL);
			}
			if ((x >= 310) && (x <= 460))
			{
				waitForIt(310, 110, 460, 150);
				can1.sendFrame(ARM2_CONTROL, setHomeID);
			}
		}
		if ((y >= 160) && (y <= 200))
		{
			if ((x >= 150) && (x <= 300))
			{
				waitForIt(150, 160, 300, 200);
				loopProgram = true;
			}
			if ((x >= 310) && (x <= 460))
			{
				waitForIt(310, 160, 460, 200);
				loopProgram = false;
			}
		}
		if ((y >= 210) && (y <= 250))
		{
			if ((x >= 150) && (x <= 300))
			{
				waitForIt(150, 210, 300, 250);
				// Memory use
				page = 11;
				hasDrawn = false;
			}
			if ((x >= 310) && (x <= 460))
			{
				waitForIt(310, 210, 460, 250);
				// Reset
				REQUEST_EXTERNAL_RESET;
			}
		}
		if ((y >= 260) && (y <= 300))
		{
			if ((x >= 150) && (x <= 300))
			{
				waitForIt(150, 260, 300, 300);
				// About 
				drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
				drawSquareBtn(135, 120, 479, 140, F("Software Development"), themeBackground, themeBackground, menuBtnColor, CENTER);
				drawSquareBtn(135, 145, 479, 165, F("Brandon Van Pelt"), themeBackground, themeBackground, menuBtnColor, CENTER);
				drawSquareBtn(135, 170, 479, 190, F("github.com/BrandonVP"), themeBackground, themeBackground, menuBtnColor, CENTER);
			}
		}
	}
}


/*=========================================================
	Framework Functions
===========================================================*/
// Only called once at startup to draw the menu
void drawMenu()
{
	// Draw Layout
	drawSquareBtn(1, 1, 480, 320, "", themeBackground, themeBackground, themeBackground, CENTER);
	drawSquareBtn(0, 1, X_PAGE_START - 1, 320, "", menuBackground, menuBackground, menuBackground, CENTER);

	// Draw Menu Buttons
	drawRoundBtn(4, 40, 122, 90, F("1-VIEW"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(4, 95, 122, 145, F("2-PROG"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(4, 150, 122, 200, F("3-MOVE"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(4, 205, 122, 255, F("4-CONF"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(4, 260, 122, 310, F("5-EXEC"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	//drawRoundBtn(5, 265, 120, 315, F("6-STOP"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// the setup function runs once when you press reset or power the board
void setup()
{
	Serial.begin(115200);
	Serial3.begin(115200);

	can1.startCAN();
	bool hasFailed = sdCard.startSD();
	(!hasFailed) ? Serial.println(F("SD failed")) : Serial.println(F("SD Running"));

	myGLCD.InitLCD();
	myGLCD.clrScr();

#if defined LI9486
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);
#elif defined LI9488
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
#endif

	// Initialize the rtc object
	rtc.begin();
#if defined UPDATE_CLOCK
	rtc.setDOW(FRIDAY);
	rtc.setTime(__TIME__);
	rtc.setDate(__DATE__);
#endif

	myGLCD.setFont(BigFont);

	myGLCD.setColor(VGA_WHITE);
	myGLCD.setBackColor(VGA_WHITE);
	myGLCD.fillRect(1, 1, 479, 319);

	// Draw the robot arm
	bmpDraw("robotarm.bmp", 0, 0);
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setBackColor(VGA_WHITE);
	myGLCD.print("Loading...", 190, 290);

	// Give 6DOF arms time to start up
	delay(2000);

	drawMenu();

	// Create program folder is it does not exist
	sdCard.createDRIVE(pDir);

	initCRC();
}

// Page control framework
void pageControl()
{
	// Poll menu buttons
	menuButtons();

	// Switch which page to load
	switch (page)
	{
	case 1: // View Page
		// Draw page
		if (!hasDrawn)
		{
			if (!drawView())
			{
				break;
			}
			axisPos.drawAxisPos(myGLCD);
			hasDrawn = true;
		}
		// Call buttons if any
		axisPos.drawAxisPosUpdate(myGLCD);
		break;
	case 2: // Program page
		// If program open jump to page 6
		if (programOpen)
		{
			page = 8;
			if (programEdit)
			{
				page = 6;
			}
			break;
		}
		// Draw page
		if (!hasDrawn)
		{
			if (!drawProgram())
			{
				break;
			}
			hasDrawn = true;
		}
		// Call buttons if any
		programButtons();
		break;
	case 3: // Move page
		// Draw page
		if (!hasDrawn)
		{
			bool isDone = drawManualControl();
			graphicLoaderState++;
			if (!isDone)
			{
				break;
			}
			hasDrawn = true;
			axisPos.drawAxisPosUpdateM(myGLCD, txIdManual, true);
		}
		// Call buttons if any
		manualControlButtons();
		axisPos.drawAxisPosUpdateM(myGLCD, txIdManual, false);
		break;
	case 4: // Configuation page
		// Draw page
		if (!hasDrawn)
		{
			if (!drawConfig())
			{
				break;
			}
			hasDrawn = true;
		}
		// Call buttons if any
		configButtons();
		break;
	case 5: // Execute
		// Draw page
		if (!hasDrawn)
		{

		}
		// Call buttons if any
		break;
	case 6: // Program edit page
		// Draw page
		if (!hasDrawn)
		{
			hasDrawn = true;
			programLoaded = true;
			drawProgramEdit();
			drawProgramEditScroll();
		}
		// Call buttons if any
		programEditButtons();
		break;
	case 7: // Error page
		// Draw page
		if (!hasDrawn)
		{
			errorMessageReturn = 0;
			hasDrawn = true;
		}
		if (errorMessageReturn == 2 || errorMessageReturn == 3)
		{
			page = oldPage;
			hasDrawn = false;
			graphicLoaderState = 0;
		}
		if (errorMessageReturn == 1)
		{
			page = oldPage;
			hasDrawn = false;
			graphicLoaderState = 0;
			programDelete();
			for (uint8_t i = 0; i < 8; i++)
			{
				fileList[selectedProgram][i] = '\0';
			}
		}
		// Call buttons if any
		errorMessageReturn = errorMSGButton(1, 2, 3);
		break;
	case 8: // Edit 
		if (!hasDrawn)
		{
			if (!drawEditPage())
			{
				break;
			}
			hasDrawn = true;
			keyIndex = 0; // Reset keyboard input index back to 0
		}
		editPageButtons();
		break;
	case 9:
		// Draw page
		if (!hasDrawn)
		{
			keyIndex = 0;
			resetKeyboard();
			drawkeyboard();
			hasDrawn = true;
		}
		keyResult = keyboardController(keyIndex);
		if (keyResult == 0xF1) // Accept
		{
			// Create dir for old filename to delete
			char temp[20] = "PROGRAMS/";
			strcat(temp, fileList[selectedProgram]);

			// Copy keyboard input to fileList name
			memcpy(fileList[selectedProgram], keyboardInput, 8);

			// Save the file under the new filename
			saveProgram();

			// Safe to delete old file name
			sdCard.deleteFile(temp);

			// Exit
			graphicLoaderState = 0;
			page = 8;
			hasDrawn = false;
		}
		if (keyResult == 0xF0) // Cancel
		{
			// Exit
			graphicLoaderState = 0;
			page = 8;
			hasDrawn = false;
		}
		break;
	case 10:
		// Draw page
		if (!hasDrawn)
		{
			drawKeypadDec();
			hasDrawn = true;
		}
		/*
* No change returns 0xFF
* Accept returns 0xF1
* Cancel returns 0xF0
* Value contained in total
*
* index: Number place
* total: Current total added value selected
*/
		keyResult = keypadControllerDec(keyIndex, totalValue);
		if (keyResult == 0xF1)
		{
			waitTime = totalValue;
			page = 6;
			hasDrawn = false;
		}
		else if (keyResult == 0xF0)
		{
			page = 6;
			hasDrawn = false;
		}
		break;
	case 11: // Memory Use
		// Draw page
		if (!hasDrawn)
		{
			memoryUse();
			hasDrawn = true;
		}
		break;
	}
}

/*============== Error Message ==============*/
// Error Message function
void drawErrorMSG(String title, String eMessage1, String eMessage2)
{
	drawSquareBtn(145, 100, 415, 220, "", menuBackground, menuBtnColor, menuBtnColor, CENTER);
	drawSquareBtn(145, 100, 415, 130, title, themeBackground, menuBtnColor, menuBtnBorder, LEFT);
	drawSquareBtn(146, 131, 414, 155, eMessage1, menuBackground, menuBackground, menuBtnText, CENTER);
	drawSquareBtn(146, 155, 414, 180, eMessage2, menuBackground, menuBackground, menuBtnText, CENTER);
	drawRoundBtn(365, 100, 415, 130, "X", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
	drawRoundBtn(155, 180, 275, 215, "Confirm", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
	drawRoundBtn(285, 180, 405, 215, "Cancel", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
}

// Notification message
void drawErrorMSG2(String title, String eMessage1, String eMessage2)
{
	drawSquareBtn(145, 100, 401, 220, "", menuBackground, menuBtnColor, menuBtnColor, CENTER);
	drawSquareBtn(145, 100, 401, 130, title, themeBackground, menuBtnColor, menuBtnBorder, LEFT);
	drawSquareBtn(146, 131, 400, 155, eMessage1, menuBackground, menuBackground, menuBtnText, CENTER);
	drawSquareBtn(146, 155, 400, 180, eMessage2, menuBackground, menuBackground, menuBtnText, CENTER);
	drawRoundBtn(365, 100, 401, 130, "X", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
}

// Error Message buttons, returns 0 by default
uint8_t errorMSGButton(uint8_t confirmPage, uint8_t cancelPage, uint8_t xPage)
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((x >= 365) && (x <= 415))
		{
			if ((y >= 100) && (y <= 130))
			{
				// X
				waitForItRect(365, 100, 415, 130);
				return xPage;
			}
		}
		if ((y >= 180) && (y <= 215))
		{
			if ((x >= 155) && (x <= 275))
			{
				// Confirm
				waitForItRect(155, 180, 275, 215);
				return confirmPage;
			}
			if ((x >= 285) && (x <= 405))
			{
				// Cancel
				waitForItRect(285, 180, 405, 215);
				return cancelPage;
			}
		}
	}
	return 0;
}


/*=========================================================
CRC - https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code
===========================================================*/
#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */

/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
 */

#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))

uint8_t  crcTable[256];

void initCRC(void)
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

uint8_t generateCRC(uint8_t const message[], int nBytes)
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

// Buttons for the main menu
void menuButtons()
{
	// Physical Button control (currently disabled)
	if (digitalRead(A4) == HIGH)
	{

	}
	if (digitalRead(A3) == HIGH)
	{

	}
	if (digitalRead(A2) == HIGH)
	{

	}
	if (digitalRead(A1) == HIGH)
	{

	}
	if (digitalRead(A0) == HIGH)
	{

	}

	// Touch screen controls
	if (Touch_getXY())
	{
		if ((x >= 4) && (x <= 122))
		{
			if ((y >= 40) && (y <= 90))
			{
				// VIEW
				waitForIt(4, 40, 122, 90);
				page = 1;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 95) && (y <= 145))
			{
				// PROG
				waitForIt(4, 95, 122, 145);
				page = 2;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 150) && (y <= 200))
			{
				// MOVE
				waitForIt(4, 150, 122, 200);
				page = 3;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 205) && (y <= 255))
			{
				// CONF
				waitForIt(4, 205, 122, 255);
				page = 4;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 260) && (y <= 310))
			{
				if (programLoaded && programRunning)
				{
					waitForIt(4, 260, 122, 310);
					programRunning = false;
					drawRoundBtn(4, 260, 122, 310, F("5-EXEC"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
					return;
				}

				if (programLoaded && !programRunning)
				{
					waitForIt(4, 260, 122, 310);
					programRunning = true;
					programProgress = 0;
					Arm1Ready = true;
					Arm2Ready = true;
					drawRoundBtn(4, 260, 122, 310, F("6-STOP"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				}
			}
		}
	}
}

// Runs the program currently loaded
void executeProgram()
{
	// Return unless enabled
	if (programRunning == false)
	{
		return;
	}
	uint8_t crc = 0;
	if (( ( Arm1Ready == true ) && ( runList.get(programProgress )->getID() == ARM1_PROGRAM ) ) || ( ( Arm2Ready == true ) && ( runList.get(programProgress)->getID() == ARM2_PROGRAM ) ))
	{
		uint8_t data[8];
		uint16_t a1 = runList.get(programProgress)->getA1();
		uint16_t a2 = runList.get(programProgress)->getA2();
		uint16_t a3 = runList.get(programProgress)->getA3();
		uint16_t a4 = runList.get(programProgress)->getA4();
		uint16_t a5 = runList.get(programProgress)->getA5();
		uint16_t a6 = runList.get(programProgress)->getA6();

		// TODO: Add grip value after the grip function is updated
		uint8_t grip = 0;
		crc = (a1 % 2) + (a2 % 2) + (a3 % 2) + (a4 % 2) + (a5 % 2) + (a6 % 2) + (grip % 2) + 1;

		data[6] = (a1 & 0xFF);
		data[5] = (a1 >> 8);
		data[5] |= ((a2 & 0xFF) << 1);
		data[4] = (a2 >> 7);
		data[4] |= ((a3 & 0x7F) << 2);
		data[3] = (a3 >> 6);
		data[3] |= ((a4 & 0x3F) << 3);
		data[2] = (a4 >> 5);
		data[2] |= ((a5 & 0x1F) << 4);
		data[1] = (a5 >> 4);
		data[1] |= ((a6 & 0xF) << 5);
		data[0] = (a6 >> 3);
		data[0] |= ((grip & 0x7) << 6);
		data[7] = generateCRC(data, 7);
		
		can1.sendFrame(runList.get(programProgress)->getID(), data);

		// Wait frame
		uint8_t executeWait[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		executeWait[COMMAND_BYTE] = SET_WAIT_TIMER;
		executeWait[SET_MIN_BYTE] = 0;
		executeWait[SET_SEC_BYTE] = runList.get(programProgress)->getWait();
		executeWait[SET_MS_BYTE] = 0;
		executeWait[CRC_BYTE] = generateCRC(executeWait, 7);

		if (runList.get(programProgress)->getID() == ARM1_PROGRAM)
		{
			can1.sendFrame(ARM1_CONTROL, executeWait);
		}
		else if (runList.get(programProgress)->getID() == ARM2_PROGRAM)
		{
			can1.sendFrame(ARM1_CONTROL, executeWait);
		}

		// Grip on/off or hold based on current and next state
		// If there was a change in the grip bool
		uint8_t executeMove[8] = { 0, EXECUTE_PROGRAM, 0, 0, 0, MOVE_GRIP, 0, 0 };
		executeMove[GRIP_BYTE] = HOLD_GRIP;
		executeMove[CRC_BYTE] = generateCRC(executeMove, 7);

		if (runList.get(programProgress)->getGrip() == 0)
		{
			executeMove[GRIP_BYTE] = OPEN_GRIP;
		}
		else if (runList.get(programProgress)->getGrip() == 1)
		{
			executeMove[GRIP_BYTE] = SHUT_GRIP;
		}

		// Send third frame with grip and execute command
		if (runList.get(programProgress)->getID() == ARM1_PROGRAM)
		{
			can1.sendFrame(ARM1_CONTROL, executeMove);
			Arm1Ready = false;
			programProgress++;
		}
		else if (runList.get(programProgress)->getID() == ARM2_PROGRAM)
		{
			can1.sendFrame(ARM2_CONTROL, executeMove);
			Arm2Ready = false;
			programProgress++;
		}
	}
	else
	{
		return;
	}

	if ((loopProgram == false) && (programProgress == runList.size()))
	{
		programRunning = false;
		drawExecuteButton();
	}
	else if ((loopProgram == true) && (programProgress == runList.size()))
	{
		programProgress = 0;
		programRunning = true;
	}
}

// Displays time on menu
void updateTime()
{
	if (millis() - updateClock > 1000)
	{
		char time[40];
		drawSquareBtn(1, 10, 125, 30, rtc.getTimeStr(), menuBackground, menuBackground, menuBtnText, CENTER);
		rtc.getTimeStr();
		updateClock = millis();
	}
}

// Called from main loop. Can also be called from any blocking code.
void backgroundProcess()
{
	executeProgram();
	updateTime();
	can1.processFrame(axisPos, myGLCD);
}

// Calls pageControl with a value of 1 to set view page as the home page
void loop()
{
	// GUI
	pageControl();

	// Background Processes
	backgroundProcess();
}