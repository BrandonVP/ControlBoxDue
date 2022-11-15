/*
 ===========================================================================
 Name        : KeyInput.h
 Author      : Brandon Van Pelt
 Created	 : 11/08/2022 9:57:18 AM
 Description : User key inputs
 ===========================================================================
 */

#include "Common.h"

#ifndef _KEYINPUT_H
#define _KEYINPUT_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef _KEYINPUT_C

char keyboardInput[9];
uint8_t keypadInput[4] = { 0, 0, 0, 0 };

void resetKeypad();
void drawKeypad();
int keypadButtons();
uint8_t keypadController(uint8_t&, uint16_t&);
void drawKeypadDec();
int keypadButtonsDec();
uint8_t keypadControllerDec(uint8_t&, uint16_t&);
void drawkeyboard();
int keyboardButtons();
uint8_t keyboardController(uint8_t&);

#else

extern char keyboardInput[9];
extern uint8_t keypadInput[4];

extern void resetKeypad();
extern void resetKeyboard();
extern void drawKeypad();
extern int keypadButtons();
extern uint8_t keypadController(uint8_t&, uint16_t&);
extern void drawKeypadDec();
extern int keypadButtonsDec();
extern uint8_t keypadControllerDec(uint8_t&, uint16_t&);
extern void drawkeyboard();
extern int keyboardButtons();
extern uint8_t keyboardController(uint8_t&);

#endif // _KEYINPUT_C
#endif // _KEYINPUT_H
