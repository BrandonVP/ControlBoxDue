#pragma once
#include "definitions.h"
#include "CANBusWiFi.h"
#include <SD.h>
#include "SDCard.h"
#include <UTFT.h>
#include "CANBus.h"
#include "AxisPos.h"

//#define DEBUG(x)  SerialUSB.print(x);
//#define DEBUGLN(x)  SerialUSB.println(x);
#define DEBUG(x)  Serial.print(x);
#define DEBUGLN(x)  Serial.println(x);
//#define DEBUG(x)
//#define DEBUGLN(x)

class Program;
class AxisPos;
class UTFT;
class CANBus;

#ifndef COMMON_H
#define COMMON_H

// How many programs can be saved to SD card
#define MAX_PROGRAMS 16

extern bool Arm1Ready;
extern bool Arm2Ready;
extern uint8_t page;
extern String programNames_G[MAX_PROGRAMS];
extern uint8_t numberOfPrograms;
extern char fileList[MAX_PROGRAMS][8];
extern uint8_t generateBitCRC(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t grip);
#endif