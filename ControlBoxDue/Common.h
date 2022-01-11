#pragma once

//#define DEBUG(x)  SerialUSB.print(x);
//#define DEBUGLN(x)  SerialUSB.println(x);
#define DEBUG(x)  Serial.print(x);
#define DEBUGLN(x)  Serial.println(x);
//#define DEBUG(x)
//#define DEBUGLN(x)

#ifndef COMMON_H
#define COMMON_H

#define MAX_PROGRAMS 10
extern String programNames_G[MAX_PROGRAMS];
extern uint8_t numberOfPrograms;
extern char fileList[20][9];


#endif