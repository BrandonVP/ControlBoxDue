/*
 ===========================================================================
 Name        : SDCard.h
 Author      : Brandon Van Pelt
 Created	 :
 Description : SDCard class manages the SD card reader hardware
 ===========================================================================
 */

#include <LinkedList.h>
#include "Program.h"

#ifndef _SDCard_H
#define _SDCard_H
#define SD_CARD_CS 8

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class SDCard
{
protected:
	typedef char MyArray[16][8];

public:
	bool startSD();
	void writeFile(String, uint16_t);
	void writeFile(String, String);
	void writeFile(char*, int, int);
	void writeFile(char*, String);
	void writeFileln(String);
	void deleteFile(String);
	void readFile(String, LinkedList<Program*>&);
	bool fileExists(String filename);
	void writeProgramName(String);
	void createDRIVE(String);
	uint8_t printDirectory(File dir, MyArray& list);
};
#endif // _SDCard_H