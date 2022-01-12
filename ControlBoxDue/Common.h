#pragma once

//#define DEBUG(x)  SerialUSB.print(x);
//#define DEBUGLN(x)  SerialUSB.println(x);
#define DEBUG(x)  Serial.print(x);
#define DEBUGLN(x)  Serial.println(x);
//#define DEBUG(x)
//#define DEBUGLN(x)

#ifndef COMMON_H
#define COMMON_H

// How many programs can be saved to SD card
#define MAX_PROGRAMS 16

extern String programNames_G[MAX_PROGRAMS];
extern uint8_t numberOfPrograms;
extern char fileList[MAX_PROGRAMS][8];

#endif