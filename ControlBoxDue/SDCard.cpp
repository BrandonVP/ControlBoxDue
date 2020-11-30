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

void SDCard::writeFile(String incoming)
{
    // File created and opened for writing
    myFile = SD.open("CANLOG.txt", FILE_WRITE);  

    // Check if file was sucsefully open
    if (myFile)        
    {
        myFile.print(incoming);
        myFile.close();
    }
    return;
}

void SDCard::writeFile(int incoming)
{
    // File created and opened for writing
    myFile = SD.open("CANLOG.txt", FILE_WRITE); 

    // Check if file was sucsefully open
    if (myFile)      
    {
        myFile.print(incoming);
        myFile.close();
    }
    return;
}

void SDCard::writeFile(uint8_t incoming, int base)
{
    // File created and opened for writing
    myFile = SD.open("CANLOG.txt", FILE_WRITE); 

    // Check if file was sucsefully open
    if (myFile)        
    {
        myFile.print(incoming, base);
        myFile.close();
    }
    return;
}

void SDCard::writeFileln()
{
    // File created and opened for writing
    myFile = SD.open("CANLOG.txt", FILE_WRITE);

    // Check if file was sucsefully open
    if (myFile) 
    {
        myFile.println(" ");
        myFile.close();
    }
    return;
}

String SDCard::readFile()
{
    return"";
}