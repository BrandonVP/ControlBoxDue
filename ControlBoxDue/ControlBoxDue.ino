/*
 Name:    ControlBoxDue.ino
 Created: 11/15/2020 8:27:18 AM
 Author:  Brandon Van Pelt
*/

/*=========================================================
	Todo List
===========================================================
Assign physical buttons
Switch between LI9486 & LI9488
Swtich between Due & mega2560
===========================================================
	End Todo List
=========================================================*/

#include "CANBusWiFi.h"
#include <LinkedList.h>
#include <SD.h>
#include <SPI.h>
#include "AxisPos.h"
#include "CANBus.h"
#include "SDCard.h"
#include "Program.h"
#include "definitions.h"
#include "icons.h"
#include "Common.h"


/*=========================================================
	Settings
===========================================================*/
// Select display
//#define LI9486
#define LI9488

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
#include <UTFT.h>
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

// Select debug Due
//#define DEBUG(x)  SerialUSB.print(x);
#define DEBUG(x)  Serial.print(x);
//#define DEBUG(x)

// For touch controls
int x, y;

// Object to control CAN Bus hardware
CANBus can1;

// Object to control SD Card Hardware
SDCard sdCard;

// AxisPos object used to get current angles of robot arm
AxisPos axisPos;

// Linked list of nodes for a program
LinkedList<Program*> runList = LinkedList<Program*>();

// Keeps track of current page
uint8_t controlPage = 1;
uint8_t page = 1;
uint8_t nextPage = 1;
uint8_t oldPage = 1;
bool hasDrawn = false;
uint8_t graphicLoaderState = 0;
uint8_t errorMessageReturn = 2;

// Current selected program
uint8_t selectedProgram = 0;

// CAN message ID and frame, value can be changed in manualControlButtons
uint16_t txIdManual = ARM1_MANUAL;

// Execute variables
bool programLoaded = false;
bool loopProgram = true;
bool programRunning = false;
bool Arm1Ready = false;
bool Arm2Ready = false;
// THIS WILL LIMIT THE SIZE OF A PROGRAM TO 255 MOVEMENTS
uint8_t programProgress = 0;

// Used to determine which PROG page should be loaded when button is pressed
bool programOpen = false;
bool programEdit = false;

// 0 = open, 1 = close, 2 = no change
int8_t gripStatus = 2;


// Timer for current angle updates
uint32_t timer = 0;

// Key input variables
char keyboardInput[8] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8_t keypadInput[4] = { 0, 0, 0, 0 };
uint8_t keyIndex = 0;

String programNames_G[MAX_PROGRAMS] = { "", "", "", "", "", "", "", "", "", "" };
uint8_t numberOfPrograms = 0;
uint8_t state = 0;
uint8_t keyResult = 0;
/*=========================================================
	Framework Functions
===========================================================*/
//
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

// Custom bitmap
void print_icon(int x, int y, const unsigned char icon[])
{
	myGLCD.setColor(menuBtnColor);
	myGLCD.setBackColor(themeBackground);
	int i = 0, row, column, bit, temp;
	int constant = 1;
	for (row = 0; row < 40; row++)
	{
		for (column = 0; column < 5; column++)
		{
			temp = icon[i];
			for (bit = 7; bit >= 0; bit--)
			{
				if (temp & constant) { myGLCD.drawPixel(x + (column * 8) + (8 - bit), y + row); }
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
		delay(80);
	}
	myGLCD.setColor(menuBtnBorder);
	myGLCD.drawRect(x1, y1, x2, y2);
}

bool Touch_getXY(void)
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
void drawManualControl()
{
	switch (graphicLoaderState)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	case 9:
		break;
	case 10:
		break;
	}
	// Clear LCD to be written 
	drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

	// Print arm logo
	print_icon(435, 5, robotarm);

	// Print page title
	drawSquareBtn(180, 10, 400, 45, F("Manual Control"), themeBackground, themeBackground, menuBtnColor, CENTER);

	// Manual control axis labels
	int j = 1;
	for (int i = 146; i < (480 - 45); i = i + 54) {
		myGLCD.setColor(menuBtnColor);
		myGLCD.setBackColor(themeBackground);
		myGLCD.printNumI(j, i + 20, 60);
		j++;
	}

	// Draw the upper row of movement buttons
	for (int i = 146; i < (480 - 54); i = i + 54) {
		drawSquareBtn(i, 80, i + 54, 140, F("/\\"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	}

	// Draw the bottom row of movement buttons
	for (int i = 146; i < (480 - 54); i = i + 54) {
		drawSquareBtn(i, 140, i + 54, 200, F("\\/"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	}

	// Draw Select arm buttons
	drawSquareBtn(165, 225, 220, 265, F("Arm"), themeBackground, themeBackground, menuBtnColor, CENTER);
	if (txIdManual == ARM1_MANUAL)
	{
		drawSquareBtn(146, 260, 200, 315, F("1"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
		drawSquareBtn(200, 260, 254, 315, F("2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	}
	else if (txIdManual == ARM2_MANUAL)
	{
		drawSquareBtn(146, 260, 200, 315, F("1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		drawSquareBtn(200, 260, 254, 315, F("2"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
	}

	// Draw grip buttons
	drawSquareBtn(270, 225, 450, 265, F("Gripper"), themeBackground, themeBackground, menuBtnColor, CENTER);
	drawSquareBtn(270, 260, 360, 315, F("Open"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(360, 260, 450, 315, F("Close"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// Draw page button function
void manualControlButtons()
{
	// Mutiply is a future funtion to allow movement of multiple angles at a time instead of just 1
	int multiply = 1;

	// Enables revese
	uint8_t reverse = 0x10;

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
				waitForItRect(146, 80, 200, 140, txIdManual, data);
				data[1] = 0;
			}
			// A2 Up
			if ((x >= 200) && (x <= 254))
			{
				data[2] = 1 * multiply;
				waitForItRect(200, 80, 254, 140, txIdManual, data);
				data[2] = 0;
			}
			// A3 Up
			if ((x >= 254) && (x <= 308))
			{
				data[3] = 1 * multiply;
				waitForItRect(254, 80, 308, 140, txIdManual, data);
				data[3] = 0;
			}
			// A4 Up
			if ((x >= 308) && (x <= 362))
			{
				data[4] = 1 * multiply;
				waitForItRect(308, 80, 362, 140, txIdManual, data);
				data[4] = 0;
			}
			// A5 Up
			if ((x >= 362) && (x <= 416))
			{
				data[5] = 1 * multiply;
				waitForItRect(362, 80, 416, 140, txIdManual, data);
				data[5] = 0;
			}
			// A6 Up
			if ((x >= 416) && (x <= 470))
			{
				data[6] = 1 * multiply;
				waitForItRect(416, 80, 470, 140, txIdManual, data);
				data[6] = 0;
			}
		}
		if ((y >= 140) && (y <= 200))
		{
			// A1 Down
			if ((x >= 156) && (x <= 200))
			{
				data[1] = (1 * multiply) + reverse;
				waitForItRect(146, 140, 200, 200, txIdManual, data);
				data[1] = 0;
			}
			// A2 Down
			if ((x >= 200) && (x <= 254))
			{
				data[2] = (1 * multiply) + reverse;
				waitForItRect(200, 140, 254, 200, txIdManual, data);
				data[2] = 0;
			}
			// A3 Down
			if ((x >= 254) && (x <= 308))
			{
				data[3] = (1 * multiply) + reverse;
				waitForItRect(254, 140, 308, 200, txIdManual, data);
				data[3] = 0;
			}
			// A4 Down
			if ((x >= 308) && (x <= 362))
			{
				data[4] = (1 * multiply) + reverse;
				waitForItRect(308, 140, 362, 200, txIdManual, data);
				data[4] = 0;
			}
			// A5 Down
			if ((x >= 362) && (x <= 416))
			{
				data[5] = (1 * multiply) + reverse;
				waitForItRect(362, 140, 416, 200, txIdManual, data);
				data[5] = 0;
			}
			// A6 Down
			if ((x >= 416) && (x <= 470))
			{
				data[6] = (1 * multiply) + reverse;
				waitForItRect(416, 140, 470, 200, txIdManual, data);
				data[6] = 0;
			}
		}
		if ((y >= 260) && (y <= 315))
		{
			if ((x >= 146) && (x <= 200))
			{
				// Select arm 1
				drawSquareBtn(146, 260, 200, 315, F("1"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
				drawSquareBtn(200, 260, 254, 315, F("2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				txIdManual = ARM1_MANUAL;
			}
			if ((x >= 200) && (x <= 254))
			{
				// Select arm 2
				drawSquareBtn(146, 260, 200, 315, F("1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				drawSquareBtn(200, 260, 254, 315, F("2"), menuBtnText, menuBtnBorder, menuBtnColor, CENTER);
				txIdManual = ARM2_MANUAL;
			}
			if ((x >= 270) && (x <= 360))
			{
				// Grip open
				data[7] = 1 * multiply;
				waitForItRect(270, 260, 360, 315, txIdManual, data);
				data[7] = 0;
			}
			if ((x >= 360) && (x <= 450))
			{
				// Grip close
				data[7] = (1 * multiply) + reverse;
				waitForItRect(360, 260, 450, 315, txIdManual, data);
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
	drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

	// Print arm logo
	print_icon(435, 5, robotarm);

	// Draw row lables
	for (int start = 35, stop = 75, row = 1; start <= 260; start = start + 45, stop = stop + 45, row++)
	{
		String rowLable = "A" + String(row);
		drawRoundBtn(160, start, 180, stop, rowLable, themeBackground, themeBackground, menuBtnColor, CENTER);
	}

	// Boxes for current arm angles
	const uint16_t xStart = 190;
	const uint16_t xStop = 295;
	const uint8_t yStart = 35;
	const uint8_t yStop = 75;
	// Arm 1
	drawRoundBtn(xStart + 110, 5, xStop + 110, 40, F("Arm2"), themeBackground, themeBackground, menuBtnColor, CENTER);
	drawRoundBtn(xStart + 110, yStart + 0, xStop + 110, yStop + 0, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart + 110, yStart + 45, xStop + 110, yStop + 45, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart + 110, yStart + 90, xStop + 110, yStop + 90, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart + 110, yStart + 135, xStop + 110, yStop + 135, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart + 110, yStart + 180, xStop + 110, yStop + 180, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart + 110, yStart + 225, xStop + 110, yStop + 225, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	// Arm 2
	drawRoundBtn(xStart, 5, xStop, 40, F("Arm1"), themeBackground, themeBackground, menuBtnColor, CENTER);
	drawRoundBtn(xStart, yStart + 0, xStop, yStop + 0, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart, yStart + 45, xStop, yStop + 45, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart, yStart + 90, xStop, yStop + 90, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart, yStart + 135, xStop, yStop + 135, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart, yStart + 180, xStop, yStop + 180, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
	drawRoundBtn(xStart, yStart + 225, xStop, yStop + 225, DEG, menuBackground, menuBackground, menuBtnColor, RIGHT);
}

// Replaced by arm broadcasting positions instead of requesting
void updateViewPage()
{
	if (millis() - timer > REFRESH_RATE)
	{
		axisPos.sendRequest(can1);
		if (page == 1)
		{
			axisPos.drawAxisPos(myGLCD);
		}
		timer = millis();
	}
}


/*==========================================================
					Program Arm
============================================================*/
// Draws scrollable box that contains 10 slots for programs
void drawProgramScroll(int scroll)
{
	// selected position = scroll * position
	// if selected draw different color border
	uint16_t yAxis = 55;
	uint16_t xAxis = 133;
	for (int i = 0; i < 5; i++)
	{
		if (programNames_G[i + scroll].compareTo("") != 0 && sdCard.fileExists(programNames_G[i + scroll]))
		{
			drawSquareBtn(xAxis, yAxis, 284, yAxis + 40, (programNames_G[i + scroll]), menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
			drawSquareBtn(284, yAxis, 351, yAxis + 40, F("LOAD"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
			drawSquareBtn(351, yAxis, 420, yAxis + 40, F("EDIT"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
			drawSquareBtn(420, yAxis, 477, yAxis + 40, F("DEL"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		}
		else
		{
			//drawSquareBtn(xAxis, yAxis, 410, yAxis + 40, (aList[i + scroll] + "-Empty"), menuBackground, menuBtnBorder, menuBtnText, LEFT);
			drawSquareBtn(xAxis, yAxis, 478, yAxis + 40, "", menuBackground, menuBtnBorder, menuBtnText, LEFT);
		}

		yAxis += 45;
	}
}

// Draws buttons for program function
bool drawProgram(int scroll = 0)
{
	switch (graphicLoaderState)
	{
	case 0:
		break;
	case 1:
		// Clear LCD to be written
		drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
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
		drawProgramScroll(scroll);
		return true;
		break;
	}
	return false;
}

// Deletes current selected program from 
void programDelete()
{
	sdCard.deleteFile(programNames_G[selectedProgram]);
}

// Load selected program from SD card into linked list
void loadProgram()
{
	sdCard.readFile(programNames_G[selectedProgram], runList);
}

// Button functions for program page 
void programButtons()
{
	// Static so that the position is saved while this method is repeatedly called in the loop
	static int scroll = 0;

	// Touch screen controls
	if (myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if ((y >= 55) && (y <= 95) && numberOfPrograms > 0 + scroll)
		{
			selectedProgram = 0 + scroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 55, 351, 95);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 55, 420, 95);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 55, 477, 95);
				// DEL
				errorMessageReturn = 2;
				drawErrorMSG(F("Confirmation"), F("Permanently"), F("Delete File?"));
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 100) && (y <= 140) && numberOfPrograms > 1 + scroll)
		{
			selectedProgram = 1 + scroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 100, 351, 140);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 100, 420, 140);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 100, 477, 140);
				// DEL
				errorMessageReturn = 2;
				drawErrorMSG(F("Confirmation"), F("Permanently"), F("Delete File?"));
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 145) && (y <= 185) && numberOfPrograms > 2 + scroll)
		{
			selectedProgram = 2 + scroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 145, 351, 185);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 145, 420, 185);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 145, 477, 185);
				// DEL
				errorMessageReturn = 2;
				drawErrorMSG(F("Confirmation"), F("Permanently"), F("Delete File?"));
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 190) && (y <= 230) && numberOfPrograms > 3 + scroll)
		{
			selectedProgram = 3 + scroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 190, 351, 230);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 190, 420, 230);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 190, 477, 230);
				// DEL
				errorMessageReturn = 2;
				drawErrorMSG(F("Confirmation"), F("Permanently"), F("Delete File?"));
				oldPage = page;
				page = 7;
				hasDrawn = false;
			}
		}
		if ((y >= 235) && (y <= 275) && numberOfPrograms > 4 + scroll)
		{
			selectedProgram = 4 + scroll;
			if ((x >= 284) && (x <= 351))
			{
				waitForIt(284, 235, 351, 275);
				// LOAD
				runList.clear();
				loadProgram();
				programLoaded = true;
			}
			if ((x >= 351) && (x <= 420))
			{
				waitForIt(351, 235, 420, 275);
				// EDIT
				runList.clear();
				loadProgram();
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
			if ((x >= 420) && (x <= 477))
			{
				waitForIt(420, 235, 477, 275);
				// DEL
				errorMessageReturn = 2;
				drawErrorMSG(F("Confirmation"), F("Permanently"), F("Delete File?"));
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
				if (scroll > 0)
				{
					scroll--;
					drawProgramScroll(scroll);
				}
			}
			if ((x >= 401) && (x <= 477))
			{
				// Scroll down
				waitForIt(401, 280, 477, 317);
				if (scroll < 5)
				{
					scroll++;
					drawProgramScroll(scroll);
				}
			}
			if ((x >= 261) && (x <= 350))
			{
				// Edit program
				waitForItRect(261, 280, 350, 317);
				// Add
				selectedProgram = findProgramNode(); // Find an empty program slot to open
				SerialUSB.println(selectedProgram);
				runList.clear(); // Empty the linked list of any program currently loaded
				programOpen = true;
				page = 8;
				hasDrawn = false;
			}
		}
	}
}

uint8_t findProgramNode()
{
	String temp = "";
	for (uint8_t i = 0; i < MAX_PROGRAMS; i++)
	{
		if (programNames_G[i].compareTo(temp) == 0)
		{
			Serial.println(i);
			return i;
		}
	}
	return 0xFF;
}

/*==========================================================
					Edit Program
============================================================*/
void drawEditPage()
{
	// Clear LCD to be written
	drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

	print_icon(435, 5, robotarm);

	drawSquareBtn(180, 10, 400, 45, F("Edit Program"), themeBackground, themeBackground, menuBtnColor, CENTER);


	drawSquareBtn(132, 54, 478, 96, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(133, 55, 280, 95, F("Filename"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(280, 55, 477, 95, programNames_G[selectedProgram], menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	drawSquareBtn(132, 99, 478, 141, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(133, 100, 280, 140, F("Size"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(280, 100, 477, 140, String(runList.size()) + "/255", menuBackground, menuBtnBorder, menuBtnText, CENTER);

	drawSquareBtn(132, 144, 478, 186, F(""), menuBackground, menuBtnBorder, menuBtnText, CENTER);

	drawSquareBtn(132, 189, 478, 231, F("Edit Steps"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	drawSquareBtn(132, 234, 478, 276, F("Save & Exit"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	drawSquareBtn(132, 280, 478, 317, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

void editPageButtons()
{
	// Touch screen controls
	if (myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

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
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
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
void drawProgramEditScroll(uint8_t scroll = 0)
{
	myGLCD.setFont(SmallFont);
	uint8_t nodeSize = runList.size();
	//Serial.println(nodeSize);
	// Each node should be listed with all information, might need small text
	int row = 43;
	for (int i = 0; i < 6; i++)
	{
		String position = String(i + scroll);
		String a = " | ";
		String b = " ";
		String label = position + a + String(runList.get(i + scroll)->getA1()) + b + String(runList.get(i + scroll)->getA2())
			+ b + String(runList.get(i + scroll)->getA3()) + b + String(runList.get(i + scroll)->getA4()) + b + String(runList.get(i + scroll)->getA5())
			+ b + String(runList.get(i + scroll)->getA6()) + a + b + "GRIP:" + String(runList.get(i + scroll)->getGrip()) + b + "ARM:" + String(runList.get(i + scroll)->getID(), HEX);
		(i + scroll < nodeSize) ? drawSquareBtn(135, row, 410, row + 37, label, menuBackground, menuBtnBorder, menuBtnText, LEFT) : drawSquareBtn(135, row, 410, row + 37, "", menuBackground, menuBtnBorder, menuBtnText, LEFT);

		row += 37;
	}
	// To edit a node just replace with a new position
	myGLCD.setFont(BigFont);
}

// Draws buttons for edit program function
void drawProgramEdit(uint8_t scroll = 0)
{
	// Clear LCD to be written
	drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

	// Print arm logo
	print_icon(435, 5, robotarm);

	// Print page title
	drawSquareBtn(180, 10, 400, 45, F("Edit"), themeBackground, themeBackground, menuBtnColor, CENTER);

	// Scroll buttons
	myGLCD.setColor(menuBtnColor);
	myGLCD.setBackColor(themeBackground);
	drawSquareBtn(420, 95, 470, 155, F("/\\"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(420, 155, 470, 215, F("\\/"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	// Draw program edit buttons
	drawSquareBtn(128, 270, 198, 315, F("Add"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(198, 270, 268, 315, F("Ins"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(268, 270, 338, 315, F("Del"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawSquareBtn(338, 270, 408, 315, F("Wait"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	switch (gripStatus)
	{
	case 0: drawSquareBtn(408, 270, 478, 315, F("Open"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 1: drawSquareBtn(408, 270, 478, 315, F("Clos"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	case 2: drawSquareBtn(408, 270, 478, 315, F("Grip"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		break;
	}

	drawSquareBtn(420, 50, 470, 90, F("X"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);

	myGLCD.setFont(SmallFont);
	drawSquareBtn(420, 220, 470, 260, F(" Save"), menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
	myGLCD.setFont(BigFont);
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
	}
	else if (txIdManual == ARM2_MANUAL)
	{
		posArray[0] = axisPos.getA1C2();
		posArray[1] = axisPos.getA2C2();
		posArray[2] = axisPos.getA3C2();
		posArray[3] = axisPos.getA4C2();
		posArray[4] = axisPos.getA5C2();
		posArray[5] = axisPos.getA6C2();
	}

	// Create program object with array positions, grip on/off, and channel
	Program* node = new Program(posArray, gripStatus, txIdManual);

	(insert < 0) ? runList.add(node) : runList.add(insert, node);
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
	Serial.print("Saving new File to:");
	Serial.println(programNames_G[selectedProgram]);
	Serial.print("Size: ");
	Serial.println(runList.size());
	Serial.println(programNames_G[selectedProgram]);
	// Write out linkedlist data to text file
	for (uint8_t i = 0; i < runList.size(); i++)
	{
		Serial.print(".");
		sdCard.writeFile(programNames_G[selectedProgram], ",");
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA1());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA2());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA3());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA4());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA5());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getA6());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getID());
		sdCard.writeFile(programNames_G[selectedProgram], space);
		sdCard.writeFile(programNames_G[selectedProgram], runList.get(i)->getGrip());
		sdCard.writeFileln(programNames_G[selectedProgram]);
	}
	if (runList.size() == 0)
	{
		Serial.println("empty");
		sdCard.writeFileln(programNames_G[selectedProgram]);
	}
	
	saveProgramNames();
}

// Button functions for edit program page
void programEditButtons()
{
	static uint8_t selectedNode = 0;
	static uint16_t scroll = 0;

	// Touch screen controls
	if (myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if ((x >= 135) && (x <= 410))
		{
			if ((y >= 43) && (y <= 80))
			{
				waitForItRect(135, 43, 410, 80);
				//Serial.println(1 + scroll);
				selectedNode = 0 + scroll;
				drawProgramEditScroll(scroll);
			}
			if ((y >= 80) && (y <= 117))
			{
				waitForItRect(135, 80, 410, 117);
				//Serial.println(2 + scroll);
				selectedNode = 1 + scroll;
				drawProgramEditScroll(scroll);
			}
			if ((y >= 117) && (y <= 154))
			{
				waitForItRect(135, 117, 410, 154);
				//Serial.println(3 + scroll);
				selectedNode = 2 + scroll;
				drawProgramEditScroll(scroll);
			}
			if ((y >= 154) && (y <= 191))
			{
				waitForItRect(135, 154, 410, 191);
				//Serial.println(4 + scroll);
				selectedNode = 3 + scroll;
				drawProgramEditScroll(scroll);
			}
			if ((y >= 191) && (y <= 228))
			{
				waitForItRect(135, 191, 410, 228);
				//Serial.println(5 + scroll);
				selectedNode = 4 + scroll;
				drawProgramEditScroll(scroll);
			}
			if ((y >= 228) && (y <= 265))
			{
				waitForItRect(135, 228, 410, 265);
				//Serial.println(5 + scroll);
				selectedNode = 5 + scroll;
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
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
			}
			if ((y >= 95) && (y <= 155))
			{
				waitForIt(420, 95, 470, 155);
				if (scroll > 0)
				{
					scroll--;
					drawProgramEditScroll(scroll);
				}
			}
			if ((y >= 155) && (y <= 215))
			{
				waitForIt(420, 155, 470, 215);
				if (scroll < 10)
				{
					scroll++;
					drawProgramEditScroll(scroll);
				}
			}
			if ((y >= 220) && (y <= 260))
			{
				// Save program
				waitForItRect(420, 220, 470, 260);
				programDelete();
				saveProgram();
				programOpen = false;
				page = 2;
				hasDrawn = false;
				graphicLoaderState = 0;
			}
		}
		if ((y >= 270) && (y <= 315))
		{
			if ((x >= 128) && (x <= 198))
			{
				// Add node
				waitForItRect(128, 270, 198, 315);
				addNode();
				drawProgramEditScroll(scroll);
			}
			if ((x >= 198) && (x <= 268))
			{
				// Insert node
				waitForItRect(198, 270, 268, 315);
				addNode(selectedNode);
				drawProgramEditScroll(scroll);
			}
			if ((x >= 268) && (x <= 338))
			{
				// Delete node
				waitForItRect(268, 270, 338, 315);
				deleteNode(selectedNode);
				drawProgramEditScroll(scroll);
			}
			if ((x >= 338) && (x <= 408))
			{
				// wait
				waitForItRect(338, 270, 408, 315);
			}
			if ((x >= 408) && (x <= 478))
			{
				// Grip
				waitForItRect(408, 270, 478, 315);
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
					drawSquareBtn(408, 270, 478, 315, "Open", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				case 1:
					drawSquareBtn(408, 270, 478, 315, "Clos", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				case 2:
					drawSquareBtn(408, 270, 478, 315, "Grip", menuBtnColor, menuBtnBorder, menuBtnText, LEFT);
					break;
				}
			}
		}
	}
}

void saveProgramNames()
{
	String temp = "";
	sdCard.deleteFile("progName");
	for (uint8_t i = 0; i < MAX_PROGRAMS; i++)
	{
		if (programNames_G[i].compareTo(temp) != 0)
		{
			sdCard.writeFile("progName", programNames_G[i]);
			sdCard.writeFile("progName", "\n");
		}
	}
}

/*==========================================================
					Configure Arm
============================================================*/
// Draws the config page
void drawConfig()
{
	drawSquareBtn(126, 1, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	print_icon(435, 5, robotarm);
	drawSquareBtn(180, 10, 400, 45, F("Configuration"), themeBackground, themeBackground, menuBtnColor, CENTER);
	drawRoundBtn(150, 60, 300, 100, F("Home Ch1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(310, 60, 460, 100, F("Set Ch1"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(150, 110, 300, 150, F("Home Ch2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(310, 110, 460, 150, F("Set ch2"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(150, 160, 300, 200, F("Loop On"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(310, 160, 460, 200, F("Loop Off"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// Sends command to return arm to starting position
void homeArm(uint8_t* armIDArray)
{
	byte lowerAxis[8] = { 0x00, 0x00, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0x5A };
	byte upperAxis[8] = { 0x00, 0x00, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4 };
	byte executeMove[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	can1.sendFrame(armIDArray[0], lowerAxis);
	can1.sendFrame(armIDArray[1], upperAxis);
	can1.sendFrame(armIDArray[2], executeMove);
}

// Button functions for config page
void configButtons()
{
	uint8_t setHomeID[8] = { 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t* setHomeIDPtr = setHomeID;

	uint8_t arm1IDArray[3] = { ARM1_LOWER, ARM1_UPPER, ARM1_CONTROL };
	uint8_t arm2IDArray[3] = { ARM2_LOWER, ARM2_UPPER, ARM2_CONTROL };
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
				can1.sendFrame(arm1IDArray[2], setHomeIDPtr);
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
				can1.sendFrame(arm2IDArray[2], setHomeIDPtr);
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
	}
}


/*==========================================================
					Key Inputs
============================================================*/
// Call before using keypad to clear out old values from array used to shift input
void resetKeypad()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		keypadInput[i] = 0;
	}
}
/*============== Hex Keypad ==============*/
// User input keypad
void drawKeypad()
{
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	uint16_t posY = 80;
	uint8_t numPad = 0x00;

	for (uint8_t i = 0; i < 3; i++)
	{
		int posX = 145;
		for (uint8_t j = 0; j < 6; j++)
		{
			if (numPad < 0x10)
			{
				drawRoundBtn(posX, posY, posX + 50, posY + 40, String(numPad, HEX), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				posX += 55;
				numPad++;
			}
		}
		posY += 45;
	}
	drawRoundBtn(365, 170, 470, 210, F("<---"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(145, 220, 250, 260, F("Input:"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(255, 220, 470, 260, F(" "), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(145, 270, 305, 310, F("Accept"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 270, 470, 310, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// User input keypad
int keypadButtons()
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((y >= 80) && (y <= 120))
		{
			// 0
			if ((x >= 145) && (x <= 195))
			{
				waitForIt(145, 80, 195, 120);
				return 0x00;
			}
			// 1
			if ((x >= 200) && (x <= 250))
			{
				waitForIt(200, 80, 250, 120);
				return 0x01;
			}
			// 2
			if ((x >= 255) && (x <= 305))
			{
				waitForIt(255, 80, 305, 120);
				return 0x02;
			}
			// 3
			if ((x >= 310) && (x <= 360))
			{
				waitForIt(310, 80, 360, 120);
				return 0x03;
			}
			// 4
			if ((x >= 365) && (x <= 415))
			{
				waitForIt(365, 80, 415, 120);
				return 0x04;
			}
			// 5
			if ((x >= 420) && (x <= 470))
			{
				waitForIt(420, 80, 470, 120);
				return 0x05;
			}
		}
		if ((y >= 125) && (y <= 165))
		{
			// 6
			if ((x >= 145) && (x <= 195))
			{
				waitForIt(145, 125, 195, 165);
				return 0x06;
			}
			// 7
			if ((x >= 200) && (x <= 250))
			{
				waitForIt(200, 125, 250, 165);
				return 0x07;
			}
			// 8
			if ((x >= 255) && (x <= 305))
			{
				waitForIt(255, 125, 305, 165);
				return 0x08;
			}
			// 9
			if ((x >= 310) && (x <= 360))
			{
				waitForIt(310, 125, 360, 165);
				return 0x09;
			}
			// A
			if ((x >= 365) && (x <= 415))
			{
				waitForIt(365, 125, 415, 165);
				return 0x0A;
			}
			// B
			if ((x >= 420) && (x <= 470))
			{
				waitForIt(420, 125, 470, 165);
				return 0x0B;
			}
		}
		if ((y >= 170) && (y <= 210))
		{
			// C
			if ((x >= 145) && (x <= 195))
			{
				waitForIt(145, 170, 195, 210);
				return 0x0C;
			}
			// D
			if ((x >= 200) && (x <= 250))
			{
				waitForIt(200, 170, 250, 210);
				return 0x0D;
			}
			// E
			if ((x >= 255) && (x <= 305))
			{
				waitForIt(255, 170, 305, 210);
				return 0x0E;
			}
			// F
			if ((x >= 310) && (x <= 360))
			{
				waitForIt(310, 170, 360, 210);
				return 0x0F;

			}
			// Backspace
			if ((x >= 365) && (x <= 470))
			{
				waitForIt(365, 170, 470, 210);
				return 0x10;
			}
		}
		if ((y >= 270) && (y <= 310))
		{
			// Accept
			if ((x >= 145) && (x <= 305))
			{
				waitForIt(145, 270, 305, 310);
				return 0x11;

			}
			// Cancel
			if ((x >= 315) && (x <= 470))
			{
				waitForIt(315, 270, 470, 310);
				return 0x12;
			}
		}
	}
	return 0xFF;
}

/*
* No change returns 0xFF
* Accept returns 0xF1
* Cancel returns 0xF0
* Value contained in total
*/
uint8_t keypadController(uint8_t& index, uint16_t& total)
{
	uint8_t input = keypadButtons();
	if (input >= 0x00 && input < 0x10 && index < 3)
	{
		keypadInput[2] = keypadInput[1];
		keypadInput[1] = keypadInput[0];
		keypadInput[0] = input;
		total = keypadInput[0] * hexTable[0] + keypadInput[1] * hexTable[1] + keypadInput[2] * hexTable[2];
		drawRoundBtn(255, 220, 470, 260, String(total, 16), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		++index;
		return 0xFF;
	}
	if (input == 0x10)
	{
		switch (index)
		{
		case 1:
			keypadInput[0] = 0;
			break;
		case 2:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = 0;
			break;
		case 3:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = keypadInput[2];
			keypadInput[2] = 0;
			break;
		}
		total = keypadInput[0] * hexTable[0] + keypadInput[1] * hexTable[1] + keypadInput[2] * hexTable[2];
		drawRoundBtn(255, 220, 470, 260, String(total, 16), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		(index > 0) ? --index : 0;
		return 0xFF;
	}
	if (input == 0x11)
	{
		return 0xF1;
	}
	else if (input == 0x12)
	{
		return 0xF0;
	}
	return input;
}

/*============== Decimal Keypad ==============*/
// User input keypad
void drawKeypadDec()
{
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	uint16_t posY = 125;
	uint8_t numPad = 0x00;

	for (uint8_t i = 0; i < 2; i++)
	{
		int posX = 145;
		for (uint8_t j = 0; j < 6; j++)
		{
			if (numPad < 0x10)
			{
				drawRoundBtn(posX, posY, posX + 50, posY + 40, String(numPad, HEX), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				posX += 55;
				numPad++;
			}
		}
		posY += 45;
	}
	drawRoundBtn(365, 170, 470, 210, F("<---"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(145, 220, 250, 260, F("Input:"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(255, 220, 470, 260, F(" "), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(145, 270, 305, 310, F("Accept"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(315, 270, 470, 310, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// User input keypad
int keypadButtonsDec()
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((y >= 125) && (y <= 165))
		{
			// 0
			if ((x >= 145) && (x <= 195))
			{
				waitForIt(145, 125, 195, 165);
				return 0x00;
			}
			// 1
			if ((x >= 200) && (x <= 250))
			{
				waitForIt(200, 125, 250, 165);
				return 0x01;
			}
			// 2
			if ((x >= 255) && (x <= 305))
			{
				waitForIt(255, 125, 305, 165);
				return 0x02;
			}
			// 3
			if ((x >= 310) && (x <= 360))
			{
				waitForIt(310, 125, 360, 165);
				return 0x03;
			}
			// 4
			if ((x >= 365) && (x <= 415))
			{
				waitForIt(365, 125, 415, 165);
				return 0x04;
			}
			// 5
			if ((x >= 420) && (x <= 470))
			{
				waitForIt(420, 125, 470, 165);
				return 0x05;
			}
		}
		if ((y >= 170) && (y <= 210))
		{
			// 6
			if ((x >= 145) && (x <= 195))
			{
				waitForIt(145, 170, 195, 210);
				return 0x06;
			}
			// 7
			if ((x >= 200) && (x <= 250))
			{
				waitForIt(200, 170, 250, 210);
				return 0x07;
			}
			// 8
			if ((x >= 255) && (x <= 305))
			{
				waitForIt(255, 170, 305, 210);
				return 0x08;
			}
			// 9
			if ((x >= 310) && (x <= 360))
			{
				waitForIt(310, 170, 360, 210);
				return 0x09;

			}
			// Backspace
			if ((x >= 365) && (x <= 470))
			{
				waitForIt(365, 170, 470, 210);
				return 0x10;
			}
		}
		if ((y >= 270) && (y <= 310))
		{
			// Accept
			if ((x >= 145) && (x <= 305))
			{
				waitForIt(145, 270, 305, 310);
				return 0x11;

			}
			// Cancel
			if ((x >= 315) && (x <= 470))
			{
				waitForIt(315, 270, 470, 310);
				return 0x12;
			}
		}
	}
	return 0xFF;
}

/*
* No change returns 0xFF
* Accept returns 0xF1
* Cancel returns 0xF0
* Value contained in total
*/
uint8_t keypadControllerDec(uint8_t& index, uint16_t& total)
{
	uint8_t input = keypadButtonsDec();
	if (input >= 0x00 && input < 0x10 && index < 4)
	{
		keypadInput[3] = keypadInput[2];
		keypadInput[2] = keypadInput[1];
		keypadInput[1] = keypadInput[0];
		keypadInput[0] = input;
		total = keypadInput[0] * 1 + keypadInput[1] * 10 + keypadInput[2] * 100 + keypadInput[3] * 1000;
		drawRoundBtn(255, 220, 470, 260, String(total), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		++index;
		return 0xFF;
	}
	if (input == 0x10)
	{
		switch (index)
		{
		case 1:
			keypadInput[0] = 0;
			break;
		case 2:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = 0;
			break;
		case 3:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = keypadInput[2];
			keypadInput[2] = 0;
			break;
		case 4:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = keypadInput[2];
			keypadInput[2] = keypadInput[3];
			keypadInput[3] = 0;
			break;
		case 5:
			keypadInput[0] = keypadInput[1];
			keypadInput[1] = keypadInput[2];
			keypadInput[2] = keypadInput[3];
			keypadInput[3] = keypadInput[4];
			keypadInput[4] = 0;
			break;
		}
		total = keypadInput[0] * 1 + keypadInput[1] * 10 + keypadInput[2] * 100 + keypadInput[3] * 1000;
		drawRoundBtn(255, 220, 470, 260, String(total), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		(index > 0) ? --index : 0;
		return 0xFF;
	}
	if (input == 0x11)
	{
		return 0xF1;
	}
	else if (input == 0x12)
	{
		return 0xF0;
	}
	return input;
}

/*============== Keyboard ==============*/
// User input keypad
void drawkeyboard()
{
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);
	//SerialUSB.println("");
	uint16_t posY = 56;
	uint8_t numPad = 0x00;

	const char keyboardInput[36] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
									 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
									 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
									 'u', 'v', 'w', 'x', 'y', 'z' };
	uint8_t count = 0;

	for (uint8_t i = 0; i < 4; i++)
	{
		int posX = 135;
		for (uint8_t j = 0; j < 10; j++)
		{
			if (count < 36)
			{
				drawRoundBtn(posX, posY, posX + 32, posY + 40, String(keyboardInput[count]), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				/* Print out button coords
				SerialUSB.print(posX);
				SerialUSB.print(" , ");
				SerialUSB.print(posY);
				SerialUSB.print(" , ");
				SerialUSB.print((posX + 32));
				SerialUSB.print(" , ");
				SerialUSB.println((posY + 40));
				*/
				posX += 34;
				count++;
			}
		}
		posY += 43;
	}
	drawRoundBtn(340, 185, 474, 225, F("<--"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(135, 230, 240, 270, F("Input:"), menuBackground, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(245, 230, 475, 270, F("filename"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(135, 275, 305, 315, F("Accept"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(310, 275, 475, 315, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
}

// User input keyboard
char keyboardButtons()
{
	// Touch screen controls
	if (Touch_getXY())
	{
		if ((y >= 56) && (y <= 96))
		{
			if ((x >= 135) && (x <= 167))
			{
				waitForIt(135, 56, 167, 96);
				// 0
				DEBUG_KEYBOARD("0");
				return 0x30;
			}
			if ((x >= 169) && (x <= 201))
			{
				waitForIt(169, 56, 201, 96);
				// 1
				DEBUG_KEYBOARD("1");
				return 0x31;
			}
			if ((x >= 203) && (x <= 235))
			{
				waitForIt(203, 56, 235, 96);
				// 2
				DEBUG_KEYBOARD("2");
				return 0x32;
			}
			if ((x >= 237) && (x <= 269))
			{
				waitForIt(237, 56, 269, 96);
				// 3
				DEBUG_KEYBOARD("3");
				return 0x33;
			}
			if ((x >= 271) && (x <= 303))
			{
				waitForIt(271, 56, 303, 96);
				// 4
				DEBUG_KEYBOARD("4");
				return 0x34;
			}
			if ((x >= 305) && (x <= 337))
			{
				waitForIt(305, 56, 337, 96);
				// 5
				DEBUG_KEYBOARD("5");
				return 0x35;
			}
			if ((x >= 339) && (x <= 371))
			{
				waitForIt(339, 56, 371, 96);
				// 6
				DEBUG_KEYBOARD("6");
				return 0x36;
			}
			if ((x >= 373) && (x <= 405))
			{
				waitForIt(373, 56, 405, 96);
				// 7
				DEBUG_KEYBOARD("7");
				return 0x37;
			}
			if ((x >= 407) && (x <= 439))
			{
				waitForIt(407, 56, 439, 96);
				// 8
				DEBUG_KEYBOARD("8");
				return 0x38;
			}
			if ((x >= 441) && (x <= 473))
			{
				waitForIt(441, 56, 473, 96);
				// 9
				DEBUG_KEYBOARD("9");
				return 0x39;
			}
		}
		if ((y >= 99) && (y <= 139))
		{
			if ((x >= 135) && (x <= 167))
			{
				waitForIt(135, 99, 167, 139);
				// a
				DEBUG_KEYBOARD("a");
				return 'a';
			}
			if ((x >= 169) && (x <= 201))
			{
				waitForIt(169, 99, 201, 139);
				// b
				DEBUG_KEYBOARD("b");
				return 'b';
			}
			if ((x >= 203) && (x <= 235))
			{
				waitForIt(203, 99, 235, 139);
				// c
				DEBUG_KEYBOARD("c");
				return 0x63;
			}
			if ((x >= 237) && (x <= 269))
			{
				waitForIt(237, 99, 269, 139);
				// d
				DEBUG_KEYBOARD("d");
				return 0x64;
			}
			if ((x >= 271) && (x <= 303))
			{
				waitForIt(271, 99, 303, 139);
				// e
				DEBUG_KEYBOARD("e");
				return 0x65;
			}
			if ((x >= 305) && (x <= 337))
			{
				waitForIt(305, 99, 337, 139);
				// f
				DEBUG_KEYBOARD("f");
				return 0x66;
			}
			if ((x >= 339) && (x <= 371))
			{
				waitForIt(339, 99, 371, 139);
				// g
				DEBUG_KEYBOARD("g");
				return 0x67;
			}
			if ((x >= 373) && (x <= 405))
			{
				waitForIt(373, 99, 405, 139);
				// h
				DEBUG_KEYBOARD("h");
				return 0x68;
			}
			if ((x >= 407) && (x <= 439))
			{
				waitForIt(407, 99, 439, 139);
				// i
				DEBUG_KEYBOARD("i");
				return 0x69;
			}
			if ((x >= 441) && (x <= 473))
			{
				waitForIt(441, 99, 473, 139);
				// j
				DEBUG_KEYBOARD("j");
				return 0x6A;
			}
		}
		if ((y >= 142) && (y <= 182))
		{
			if ((x >= 135) && (x <= 167))
			{
				waitForIt(135, 142, 167, 182);
				// k
				DEBUG_KEYBOARD("k");
				return 0x6B;
			}
			if ((x >= 169) && (x <= 201))
			{
				waitForIt(169, 142, 201, 182);
				// l
				DEBUG_KEYBOARD("l");
				return 0x6C;
			}
			if ((x >= 203) && (x <= 235))
			{
				waitForIt(203, 142, 235, 182);
				// m
				DEBUG_KEYBOARD("m");
				return 0x6D;
			}
			if ((x >= 237) && (x <= 269))
			{
				waitForIt(237, 142, 269, 182);
				// n
				DEBUG_KEYBOARD("n");
				return 0x6E;
			}
			if ((x >= 271) && (x <= 303))
			{
				waitForIt(271, 142, 303, 182);
				// o
				DEBUG_KEYBOARD("o");
				return 0x6F;
			}
			if ((x >= 305) && (x <= 337))
			{
				waitForIt(305, 142, 337, 182);
				// p
				DEBUG_KEYBOARD("p");
				return 0x70;
			}
			if ((x >= 339) && (x <= 371))
			{
				waitForIt(339, 142, 371, 182);
				// q
				DEBUG_KEYBOARD("q");
				return 0x71;
			}
			if ((x >= 373) && (x <= 405))
			{
				waitForIt(373, 142, 405, 182);
				// r
				DEBUG_KEYBOARD("r");
				return 0x72;
			}
			if ((x >= 407) && (x <= 439))
			{
				waitForIt(407, 142, 439, 182);
				// s
				DEBUG_KEYBOARD("s");
				return 0x73;
			}
			if ((x >= 441) && (x <= 473))
			{
				waitForIt(441, 142, 473, 182);
				// t
				DEBUG_KEYBOARD("t");
				return 0x74;
			}
		}
		if ((y >= 185) && (y <= 225))
		{
			if ((x >= 135) && (x <= 167))
			{
				waitForIt(135, 185, 167, 225);
				// u
				DEBUG_KEYBOARD("u");
				return 0x75;
			}
			if ((x >= 169) && (x <= 201))
			{
				waitForIt(169, 185, 201, 225);
				// v
				DEBUG_KEYBOARD("v");
				return 0x76;
			}

			if ((x >= 203) && (x <= 235))
			{
				waitForIt(203, 185, 235, 225);
				// w
				DEBUG_KEYBOARD("w");
				return 0x77;
			}
			if ((x >= 237) && (x <= 269))
			{
				waitForIt(237, 185, 269, 225);
				// x
				DEBUG_KEYBOARD("x");
				return 0x78;
			}
			if ((x >= 271) && (x <= 303))
			{
				waitForIt(271, 185, 303, 225);
				// y
				DEBUG_KEYBOARD("y");
				return 0x79;
			}
			if ((x >= 305) && (x <= 337))
			{
				waitForIt(305, 185, 337, 225);
				// z
				DEBUG_KEYBOARD("z");
				return 0x7A;
			}
			if ((x >= 340) && (x <= 474))
			{
				waitForIt(340, 185, 474, 225);
				// Backspace
				DEBUG_KEYBOARD("Backspace");
				return 0xF2;
			}
		}
		if ((y >= 275) && (y <= 315))
		{
			if ((x >= 135) && (x <= 305))
			{
				waitForIt(135, 275, 305, 315);
				// Accept
				DEBUG_KEYBOARD("Accept");
				return 0xF1;

			}
			if ((x >= 310) && (x <= 475))
			{
				waitForIt(310, 275, 475, 315);
				// Cancel
				DEBUG_KEYBOARD("Cancel");
				return 0xF0;
			}
		}
	}
	return 0xFF;
}

/*
* No change returns 0xFF
* Accept returns 0xF1
* Cancel returns 0xF0
* Value contained in global keyboardInput
*/
uint8_t keyboardController(uint8_t& index)
{
	uint8_t input = keyboardButtons();
	if (input > 0x29 && input < 0x7B)
	{
		if (index < 8)
		{
			keyboardInput[index] = input;
		}
		
		Serial.println(index);
		for (int i = 0; i < 8; i++)
		{
			Serial.println(keyboardInput[i]);
		}
		drawRoundBtn(245, 230, 475, 270, String(keyboardInput), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		++index;
		return 0xFF;
	}
	if (input == 0xF2)
	{
		keyboardInput[index - 1] = 0x20;
		drawRoundBtn(245, 230, 475, 270, String(keyboardInput), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		--index;
		return 0xFF;
	}
	return input;
}


/*=========================================================
	Framework Functions
===========================================================*/
// Only called once at startup to draw the menu
void drawMenu()
{
	// Draw Layout
	drawSquareBtn(1, 1, 480, 320, "", themeBackground, themeBackground, themeBackground, CENTER);
	drawSquareBtn(1, 1, 125, 320, "", menuBackground, menuBackground, menuBackground, CENTER);

	// Draw Menu Buttons
	drawRoundBtn(5, 6, 120, 53, F("1-VIEW"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(5, 58, 120, 105, F("2-PROG"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(5, 110, 120, 157, F("3-MOVE"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(5, 162, 120, 209, F("4-CONF"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(5, 214, 120, 261, F("5-EXEC"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
	drawRoundBtn(5, 266, 120, 313, F("6-STOP"), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
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
	delay(6000);

	drawMenu();

	sdCard.deleteFile("progName");
	sdCard.writeFile("progName", "a");
	sdCard.writeFile("progName", "\n");
	sdCard.writeFile("progName", "b");
	sdCard.writeFile("progName", "\n");
	sdCard.writeFileln("test");

	programNames_G[0] = "a";
	selectedProgram = 0;
	saveProgram();
	programNames_G[1] = "b";
	selectedProgram = 1;
	saveProgram();
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
			drawView();
			axisPos.drawAxisPos(myGLCD);
			axisPos.drawAxisPos(myGLCD);
			hasDrawn = true;
		}
		// Call buttons if any
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

		if (errorMessageReturn == 1)
		{
			programDelete();
			errorMessageReturn = 2;
		}
		// Draw page
		if (!hasDrawn)
		{
			if (!drawProgram())
			{
				graphicLoaderState++;
			}
			else
			{
				hasDrawn = true;
			}
		}
		// Call buttons if any
		programButtons();
		break;
	case 3: // Move page
		// Draw page
		if (!hasDrawn)
		{
			drawManualControl();
			hasDrawn = true;
		}
		// Call buttons if any
		manualControlButtons();
		break;
	case 4: // Configuation page
		// Draw page
		if (!hasDrawn)
		{
			drawConfig();
			hasDrawn = true;
		}
		// Call buttons if any
		configButtons();
		break;
	case 5: // Execute
		// Draw page
		if (!hasDrawn)
		{
			hasDrawn = true;
			if (programLoaded == true)
			{
				programRunning = true;
				programProgress = 0;
				Arm1Ready = true;
				Arm2Ready = true;
			}
			// ---ERROR MESSAGE---
			page = oldPage;
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
			hasDrawn = true;
		}
		if (errorMessageReturn == 0 || errorMessageReturn == 1)
		{
			page = oldPage;
			hasDrawn = false;
			graphicLoaderState = 0;
		}
		// Call buttons if any
		errorMSGButtons();
		break;
	case 8:
		if (!hasDrawn)
		{
			drawEditPage();
			hasDrawn = true;
			keyIndex = 0; // Reset keyboard input index back to 0
		}
		editPageButtons();
		break;
	case 9:
		// Draw page
		if (!hasDrawn)
		{
			drawkeyboard();
			hasDrawn = true;
		}
		keyResult = keyboardController(keyIndex);
		if (keyResult == 0xF1)
		{
			Serial.print("Deleteing old file: ");
			Serial.println(programNames_G[selectedProgram]);
			sdCard.deleteFile(programNames_G[selectedProgram]);
			Serial.print("Copy new name over: ");
			Serial.println(String(keyboardInput));
			programNames_G[selectedProgram] = String(keyboardInput);
			Serial.println("Save Program");
			saveProgram();
			page = 8;
			hasDrawn = false;
		}
		if (keyResult == 0xF0)
		{
			Serial.println("here");
			page = 6;
			hasDrawn = false;
		}
		break;
	}
}

// errorMSGReturn
// 0 = false
// 1 = true
// 2 = no value
// Error Message function
bool drawErrorMSG(String title, String eMessage1, String eMessage2)
{
	drawSquareBtn(145, 100, 415, 220, "", menuBackground, menuBtnColor, menuBtnColor, CENTER);
	drawSquareBtn(145, 100, 415, 130, title, themeBackground, menuBtnColor, menuBtnBorder, LEFT);
	drawSquareBtn(146, 131, 414, 155, eMessage1, menuBackground, menuBackground, menuBtnText, CENTER);
	drawSquareBtn(146, 155, 414, 180, eMessage2, menuBackground, menuBackground, menuBtnText, CENTER);
	drawRoundBtn(365, 100, 415, 130, "X", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
	drawRoundBtn(155, 180, 275, 215, "Confirm", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
	drawRoundBtn(285, 180, 405, 215, "Cancel", menuBtnColor, menuBtnColor, menuBtnText, CENTER);
}

void errorMSGButtons()
{
	if (myTouch.dataAvailable())
	{
		myTouch.read();
		x = myTouch.getX();
		y = myTouch.getY();

		if ((x >= 365) && (x <= 415))
		{
			if ((y >= 100) && (y <= 130))
			{
				waitForItRect(365, 100, 415, 130);
				errorMessageReturn = 0;
			}
		}
		if ((y >= 180) && (y <= 215))
		{
			if ((x >= 155) && (x <= 275))
			{
				waitForItRect(155, 180, 275, 215);
				errorMessageReturn = 1;

			}
			if ((x >= 285) && (x <= 405))
			{
				waitForItRect(285, 180, 405, 215);
				errorMessageReturn = 0;
			}
		}
	}
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
				page = oldPage;
			}
		}
	}
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
		if ((x >= 5) && (x <= 120))
		{
			if ((y >= 6) && (y <= 53))
			{
				// VIEW
				waitForIt(5, 6, 120, 53);
				page = 1; 
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 58) && (y <= 105))
			{
				// PROG
				waitForIt(5, 58, 120, 105);
				numberOfPrograms = 0;
				page = 2;
				graphicLoaderState = 0;
				hasDrawn = false;
				sdCard.readProgramName("progName");
				SerialUSB.println(programNames_G[0]);
				SerialUSB.println(programNames_G[1]);
				SerialUSB.println(programNames_G[2]);
				SerialUSB.println(programNames_G[3]);
			}
			if ((y >= 110) && (y <= 157))
			{
				// MOVE
				waitForIt(5, 110, 120, 157);
				page = 3;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 162) && (y <= 209))
			{
				// CONF
				waitForIt(5, 162, 120, 209);
				page = 4;
				graphicLoaderState = 0;
				hasDrawn = false;
			}
			if ((y >= 214) && (y <= 261))
			{
				// EXEC
				waitForIt(5, 214, 120, 261);
				oldPage = page;
				page = 5;
				hasDrawn = false;
			}
			if ((y >= 266) && (y <= 313))
			{
				// STOP
				waitForIt(5, 266, 120, 313);
				programRunning = false;
			}
		}
	}
}

// Watch for incoming CAN traffic
void TrafficManager()
{
	uint8_t sw_fn = can1.processFrame();
	switch (sw_fn)
	{
	case 0: // No traffic

		break;

	case 1: // C1 lower
		axisPos.updateAxisPos(can1, ARM1_RX);
		if (page == 1)
		{
			axisPos.drawAxisPos(myGLCD);
		}
		break;

	case 2: //  C1 Upper
		axisPos.updateAxisPos(can1, ARM1_RX);
		if (page == 1)
		{
			axisPos.drawAxisPos(myGLCD);
		}
		break;

	case 3: // C1 Confirmation
		Arm1Ready = true;
		Arm2Ready = true;
		Serial.println("Arm1Ready");
		break;

	case 4: // C2 Lower
		axisPos.updateAxisPos(can1, ARM2_RX);
		if (page == 1)
		{
			axisPos.drawAxisPos(myGLCD);
		}
		break;

	case 5: // C2 Upper
		axisPos.updateAxisPos(can1, ARM2_RX);
		if (page == 1)
		{
			axisPos.drawAxisPos(myGLCD);
		}
		break;

	case 6: // C2 Confirmation
		Arm1Ready = true;
		Arm2Ready = true;
		Serial.println("Arm2Ready");
		break;
	}
}

//
void executeProgram()
{
	// Return unless enabled
	if (programRunning == false)
	{
		return;
	}

	if (programProgress == runList.size())
	{
		programRunning = false;
	}
	Serial.println("here");
	if (Arm1Ready == true && Arm2Ready == true)
	{
		// CAN messages for axis movements
		uint8_t lowerAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint8_t upperAxis[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint8_t executeMove[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint16_t IDArray[3];
		uint16_t incomingID;

		if (runList.get(programProgress)->getID() == ARM1_MANUAL)
		{
			IDArray[0] = ARM1_CONTROL;
			IDArray[1] = ARM1_LOWER;
			IDArray[2] = ARM1_UPPER;
			incomingID = ARM1_RX;
		}
		if (runList.get(programProgress)->getID() == ARM2_MANUAL)
		{
			IDArray[0] = ARM2_CONTROL;
			IDArray[1] = ARM2_LOWER;
			IDArray[2] = ARM2_UPPER;
			incomingID = ARM2_RX;
		}

		// Populate CAN messages with angles from current linkedlist
		// Axis 1
		if (runList.get(programProgress)->getA1() <= 0xFF)
		{
			lowerAxis[3] = runList.get(programProgress)->getA1();
		}
		else
		{
			lowerAxis[2] = runList.get(programProgress)->getA1() - 0xFF;
			lowerAxis[3] = 0xFF;
		}
		// Axis 2
		if (runList.get(programProgress)->getA2() <= 0xFF)
		{
			lowerAxis[5] = runList.get(programProgress)->getA2();
		}
		else
		{
			lowerAxis[4] = runList.get(programProgress)->getA2() - 0xFF;
			lowerAxis[5] = 0xFF;
		}
		// Axis 3
		if (runList.get(programProgress)->getA3() <= 0xFF)
		{
			lowerAxis[7] = runList.get(programProgress)->getA3();
		}
		else
		{
			lowerAxis[6] = runList.get(programProgress)->getA3() - 0xFF;
			lowerAxis[7] = 0xFF;
		}

		// Send first frame with axis 1-3
		can1.sendFrame(IDArray[1], lowerAxis);

		// Axis 4
		if (runList.get(programProgress)->getA4() <= 0xFF)
		{
			upperAxis[3] = runList.get(programProgress)->getA4();
		}
		else
		{
			upperAxis[2] = runList.get(programProgress)->getA4() - 0xFF;
			upperAxis[3] = 0xFF;
		}
		// Axis 5
		if (runList.get(programProgress)->getA5() <= 0xFF)
		{
			upperAxis[5] = runList.get(programProgress)->getA5();
		}
		else
		{
			upperAxis[4] = runList.get(programProgress)->getA5() - 0xFF;
			upperAxis[5] = 0xFF;
		}
		// Axis 6
		if (runList.get(programProgress)->getA5() <= 0xFF)
		{
			upperAxis[7] = runList.get(programProgress)->getA6();
		}
		else
		{
			upperAxis[6] = runList.get(programProgress)->getA6() - 0xFF;
			upperAxis[7] = 0xFF;
		}

		// Send second frame with axis 4-6
		can1.sendFrame(IDArray[2], upperAxis);

		// Change to array of IDs
		uint8_t ID = runList.get(programProgress)->getID();

		// Grip on/off or hold based on current and next state
		// If there was a change in the grip bool
		executeMove[6] = 0x00;
		executeMove[7] = 0x00;

		if (runList.get(programProgress)->getGrip() == 0)
		{
			executeMove[6] = 0x01;
		}
		else if (runList.get(programProgress)->getGrip() == 1)
		{
			executeMove[7] = 0x01;
		}

		// Send third frame with grip and execute command
		can1.sendFrame(IDArray[0], executeMove);

		Arm1Ready = false;
		Arm2Ready = false;
		//Serial.print("linkedListSize: ");
		//Serial.println(programProgress);
		programProgress++;
	}

	// Loop if enabled
	if (programProgress == runList.size() && loopProgram == true)
	{
		programProgress = 0;
		programRunning = true;
	}
}

void backgroundProcess()
{
	TrafficManager();
	//updateViewPage();
	executeProgram();
}

// Calls pageControl with a value of 1 to set view page as the home page
void loop()
{
	// GUI
	pageControl();

	// Background Processes
	backgroundProcess();
}
