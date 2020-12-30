// SDCard class manages the SD card reader hardware

#include <SD.h>
#include "SDCard.h"

// File object
File myFile;

// Called at setup to initialize the SD Card
bool SDCard::startSD()
{
    if (!SD.begin(8)) {
        return false;
    }
    return true;
}

/*=========================================================
    Write File Methods
===========================================================*/
// Write string to SD Card
void SDCard::writeFile(char* filename, String incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming);
        myFile.close();
    }
    return;
}

// Write string to SD Card
void SDCard::writeFile(String filename, String incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming);
        myFile.close();
    }
    return;
}

void SDCard::writeFile(String filename, uint8_t incoming)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming);
        myFile.close();
    }
    return;
}

// Write integer and base to SD Card
void SDCard::writeFile(char* filename, int incoming, int base)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.print(incoming, base);
        myFile.close();
    }
    return;
}

// Write return to SD Card file
void SDCard::writeFileln(String filename)
{
    // File created and opened for writing
    myFile = SD.open(filename, FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile)
    {
        myFile.println(" ");
        myFile.close();
    }
    return;
}


/*=========================================================
    Delete File Methods
===========================================================*/
// Delete SD Card file
void SDCard::deleteFile(char* filename)
{
    //remove any existing file with this name
    SD.remove(filename);
}


String SDCard::readFile()
{
    return"";
}