// SDCard class manages the SD card reader hardware

#include <SD.h>
#include "SDCard.h"
#include <string>
#include "Common.h"

// File object
File myFile;

// Called at setup to initialize the SD Card
bool SDCard::startSD()
{
    if (!SD.begin(SD_CARD_CS)) {
        return false;
    }
    return true;
}

/*=========================================================
    Write File Methods
===========================================================*/
#define DEBUG_WRITEFILE_1
// Write string to SD Card
void SDCard::writeFile(char* filename, String incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_WRITEFILE_1
    DEBUGLN("void SDCard::writeFile(char* filename, String incoming)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_WRITEFILE_1

    // Check if file was sucsefully open
    if (myFile)
    {
        Serial.println("here11");
        myFile.print(incoming);
        myFile.close();
    }
}

#define DEBUG_WRITEFILE_2
// Write string to SD Card
void SDCard::writeFile(String filename, String incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_WRITEFILE_2
    DEBUGLN("void SDCard::writeFile(String filename, String incoming)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_WRITEFILE_2

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming);
        myFile.close();
    }
}

#define DEBUG_WRITEFILE_3
// Write string to SD Card
void SDCard::writeFile(String filename, uint8_t incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_WRITEFILE_3
    DEBUGLN("void SDCard::writeFile(String filename, String incoming)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_WRITEFILE_3

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming);
        myFile.close();
    }
}

#define DEBUG_WRITEFILE_4
// Write integer and base to SD Card
void SDCard::writeFile(char* filename, int incoming, int base)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_WRITEFILE_4
    DEBUGLN("void SDCard::writeFile(char* filename, int incoming, int base)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_WRITEFILE_4

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming, base);
        myFile.close();
    }
}

#define DEBUG_WRITEFILELN
// Write return to SD Card file
void SDCard::writeFileln(String filename)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_WRITEFILELN
    DEBUGLN("void SDCard::writeFileln(String filename)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_WRITEFILE_4

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.println(" ");
        myFile.close();
    }
}

#define DEBUG_PROGRAM_NAMES
// Saves program file names to a file
void SDCard::writeProgramName(String filename)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

#ifdef DEBUG_PROGRAM_NAMES
    DEBUGLN("void SDCard::writeProgramName(String filename)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_PROGRAM_NAMES

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.println(" ");
        myFile.close();
    }
}

/*=========================================================
    Delete File Methods
===========================================================*/
#define DEBUG_DELETE_FILE
// Delete SD Card file
void SDCard::deleteFile(String filename)
{
    //remove any existing file with this name
    bool temp = SD.remove(filename);

#ifdef DEBUG_DELETE_FILE
    DEBUGLN("void SDCard::deleteFile(String filename)");
    DEBUG("Deleting File: ");
    DEBUGLN(filename);
    DEBUG("Did it delete? ");
    DEBUGLN(temp);
#endif // DEBUG_DELETE_FILE
}

#define DEBUG_FILE_EXISTS
// Check if file exists
bool SDCard::fileExists(String filename)
{
    myFile = SD.open(filename);
    bool value = myFile.available();
    myFile.close();

#ifdef DEBUG_FILE_EXISTS
    DEBUGLN("bool SDCard::fileExists(String filename)");
    DEBUG("Checking File: ");
    DEBUGLN(filename);
    DEBUG("Does it exist? ");
    DEBUGLN(value);
#endif // DEBUG_FILE_EXISTS

    return value;
}

/*=========================================================
    Read File Methods
===========================================================*/
#define DEBUG_PROGRAM_NAMES
// Read in the file names
void SDCard::readProgramName(String filename)
{
    uint8_t i = 0;
    //String tempStr;
    myFile = SD.open(filename);

#ifdef DEBUG_PROGRAM_NAMES
    DEBUGLN("void SDCard::readProgramName(String filename)");
    DEBUG("Opening File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_PROGRAM_NAMES

    while (myFile.available()) 
    {
        programNames_G[i] = myFile.readStringUntil('\n');
        numberOfPrograms++;
        i++;
    }
}

#define DEBUG_READ_FILE
// Read in program from SD
void SDCard::readFile(String filename, LinkedList<Program*> &runList)
{
    String tempStr;
    uint16_t posArray[6];
    uint8_t channel;
    uint8_t grip;
    char c[20];
    myFile = SD.open(filename);

#ifdef DEBUG_READ_FILE
    DEBUGLN("void SDCard::readFile(String filename, LinkedList<Program*> &runList)");
    DEBUG("Deleting File: ");
    DEBUGLN(filename);
    DEBUG("Did it open? ");
    DEBUGLN(bool(myFile));
#endif // DEBUG_READ_FILE

    myFile.readStringUntil(',');
    while (myFile.available()) {
        for (int i = 0; i < 8; i++)
        {
            tempStr = (myFile.readStringUntil(','));
           strcpy(c, tempStr.c_str());
           if (i < 6)
           {
               posArray[i] = atoi(c);
           }
           if (i == 6)
           {
               channel = atoi(c);
           }
           if (i == 7)
           {
               grip = atoi(c);
           }
           
        }
        for (uint8_t i = 0; i < 6; i++)
        {
            Serial.println(posArray[i]);
        }
            Serial.println(channel);
            Serial.println(grip);
            Serial.println("");
        
            Program* node = new Program(posArray, grip, channel);
            runList.add(node);
    }
    myFile.close();
}