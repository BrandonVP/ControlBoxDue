/*
 ===========================================================================
 Name        : Configuration.cpp
 Author      : Brandon Van Pelt
 Created	 : 11/16/2022
 Description : Configure page
 ===========================================================================
 */

#include "Configuration.h"
#include "icons.h"

 // https://forum.arduino.cc/t/due-software-reset/332764/5
 //Defines so the device can do a self reset
#define SYSRESETREQ    (1<<2)
#define VECTKEY        (0x05fa0000UL)
#define VECTKEY_MASK   (0x0000ffffUL)
#define AIRCR          (*(uint32_t*)0xe000ed0cUL) // fixed arch-defined address
#define REQUEST_EXTERNAL_RESET (AIRCR=(AIRCR&VECTKEY_MASK)|VECTKEY|SYSRESETREQ)

#define _CONFIGURATION_C

bool loopProgram = true;

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
	byte cmd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	cmd[COMMAND_BYTE] = HOME_AXIS_POSITION;
	can1.sendFrame(arm_control, cmd);
}

// Button functions for config page
void configButtons()
{
	uint8_t setHomeID[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	setHomeID[COMMAND_BYTE] = RESET_AXIS_POSITION;
	// Touch screen controls
	if (Touch_getXY())
	{
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
				memoryUse();
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

