/*
 ===========================================================================
 Name        : Common.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : Shared resources
 ===========================================================================
 */

#pragma once
//#include "CANBusWiFi.h"
#include <SD.h>
#include <UTFT.h>
#include "SDCard.h"
#include "CANBus.h"
#include "AxisPos.h"

//#define DEBUG(x)  SerialUSB.print(x);
//#define DEBUGLN(x)  SerialUSB.println(x);
#define DEBUG(x)  Serial.print(x);
#define DEBUGLN(x)  Serial.println(x);
//#define DEBUG(x)
//#define DEBUGLN(x)

// Global LCD theme color variables
#define themeBackground 0xFFFF // White
#define menuBtnText 0xFFFF // White
#define menuBtnBorder 0x0000 // Black
#define menuBtnColor 0xFC00 // Orange
#define menuBackground 0xC618 //Silver

//class Program;
//class AxisPos;
//class UTFT;
//class CANBus;

//#ifndef COMMON_H
//#define COMMON_H

// How many programs can be saved to SD card
#define MAX_PROGRAMS 16

// For touch controls
extern int x, y;

extern const PROGMEM uint32_t hexTable[8];
extern void drawErrorMSG2(String title, String eMessage1, String eMessage2);
extern void drawErrorMSG2(String, String, String);
extern void drawRoundBtn(int, int, int, int, String, int, int, int, int);
extern void drawSquareBtn(int, int, int, int, String, int, int, int, int);
extern void waitForIt(int, int, int, int);
extern bool Touch_getXY();
extern bool Arm1Ready;
extern bool Arm2Ready;
extern uint8_t page;
extern String programNames_G[MAX_PROGRAMS];
extern uint8_t numberOfPrograms;
extern char fileList[MAX_PROGRAMS][8];
extern uint8_t generateCRC(uint8_t const message[], int nBytes);
//#endif