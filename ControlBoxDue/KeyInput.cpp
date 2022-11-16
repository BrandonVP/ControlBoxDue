/*
 ===========================================================================
 Name        : KeyInput.cpp
 Author      : Brandon Van Pelt
 Created	 : 11/08/2022
 Description : USer key inputs
 ===========================================================================
 */

#define _KEYINPUT_C

 //#define DEBUG_KEYBOARD(x)  SerialUSB.println(x);
#define DEBUG_KEYBOARD(x)

#include "KeyInput.h"
#include "definitions.h"

// Call before using keypad to clear out old values from array
void resetKeypad()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		keypadInput[i] = 0;
	}
}

// Call before using keboard to clear out old values from array
void resetKeyboard()
{
	for (uint8_t i = 0; i < 9; i++)
	{
		keyboardInput[i] = '\0';
	}
}

/*============== Hex Keypad ==============*/
// User input keypad
void drawKeypad()
{
	uint16_t posY = 80;
	uint8_t numPad = 0x00;

	// Clear LCD
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

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
				return 0xF1;

			}
			// Cancel
			if ((x >= 315) && (x <= 470))
			{
				waitForIt(315, 270, 470, 310);
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
* Value contained in total
*
* index: Number place
* total: Current total added value selected
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
	else if (input == 0x10)
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

	return input;
}

/*============== Decimal Keypad ==============*/
// User input keypad
void drawKeypadDec()
{
	uint16_t posY = 125;
	uint8_t numPad = 0x00;

	// Clear LCD
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, CENTER);

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
				return 0xF1;

			}
			// Cancel
			if ((x >= 315) && (x <= 470))
			{
				waitForIt(315, 270, 470, 310);
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
* Value contained in total
*
* index: Number place
* total: Current total added value selected
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
	else if (input == 0x10)
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

	return input;
}

/*============== Keyboard ==============*/
// User input keypad
void drawkeyboard()
{
	resetKeyboard();
	uint16_t posY = 56;
	uint8_t numPad = 0x00;
	uint8_t count = 0;
	const char alpha[37] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
							 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
							 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
							 'u', 'v', 'w', 'x', 'y', 'z', '_' };

	// Clear LCD
	drawSquareBtn(131, 55, 479, 319, "", themeBackground, themeBackground, themeBackground, 2);

	for (uint8_t i = 0; i < 4; i++)
	{
		int posX = 135;
		for (uint8_t j = 0; j < 10; j++)
		{
			if (count < 37)
			{
				drawRoundBtn(posX, posY, posX + 32, posY + 40, String(alpha[count]), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
				posX += 34;
				count++;
			}
		}
		posY += 43;
	}

	drawRoundBtn(373, 185, 475, 225, F("<--"), menuBtnColor, menuBtnBorder, menuBtnText, 2);
	drawRoundBtn(135, 230, 240, 270, F("Input:"), menuBackground, menuBtnBorder, menuBtnText, 2);
	drawRoundBtn(245, 230, 475, 270, F(""), menuBtnColor, menuBtnBorder, menuBtnText, 2);
	drawRoundBtn(135, 275, 305, 315, F("Accept"), menuBtnColor, menuBtnBorder, menuBtnText, 2);
	drawRoundBtn(310, 275, 475, 315, F("Cancel"), menuBtnColor, menuBtnBorder, menuBtnText, 2);
}

// User input keyboard
int keyboardButtons()
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
				return 0x61;
			}
			if ((x >= 169) && (x <= 201))
			{
				waitForIt(169, 99, 201, 139);
				// b
				DEBUG_KEYBOARD("b");
				return 0x62;
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
			if ((x >= 339) && (x <= 371))
			{
				waitForIt(339, 185, 371, 225);
				// z
				DEBUG_KEYBOARD("_");
				return 0x5F;
			}
			if ((x >= 373) && (x <= 474))
			{
				waitForIt(373, 185, 475, 225);
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
*
* index: Letter place (0-7)
*/
uint8_t keyboardController(uint8_t& index)
{
	uint8_t input = keyboardButtons();

	if (input > 0x29 && input < 0x7B && index < 8) // 8 is max size of a filename
	{
		keyboardInput[index] = input;
		drawRoundBtn(245, 230, 475, 270, String(keyboardInput), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		++index;
		return 0xFF;
	}
	else if (input == 0xF2 && index > 0)
	{
		keyboardInput[index - 1] = 0x20;
		drawRoundBtn(245, 230, 475, 270, String(keyboardInput), menuBtnColor, menuBtnBorder, menuBtnText, CENTER);
		--index;
		return 0xFF;
	}

	return input;
}