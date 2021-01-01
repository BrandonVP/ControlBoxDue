// SDCard class manages the SD card reader hardware

#include <SD.h>
#include "SDCard.h"
#include <string>


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
void SDCard::deleteFile(String filename)
{
    //remove any existing file with this name
    SD.remove(filename);
}


// Modified SdFat library code to read field in text file from sd
void SDCard::readFile(String filename, LinkedList<Program*> &runList)
{
    String tempStr;
    uint16_t posArray[6];
    uint8_t channel;
    bool grip;
    char c[20];
    myFile = SD.open(filename);
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

    /*
    uint8_t size = 10;
    char str[10];
    char ch;
    char* ptr;
    size_t n = 0;
    const char* delim = ",";
    while (myFile.available()) {
        while ((n + 1) < size && myFile.read(&ch, 1) == 1) {

            str[n++] = ch;
            if (strchr(delim, ch)) {
                break;
            }
        }
        str[n] = '\0';
        Serial.println(strtol(str, &ptr, 16));
    }
    */



    /*
    myFile = SD.open(filename);
    if (myFile) {
        Serial.println(filename);

        // read from the file until there's nothing else in it:
        while (myFile.available()) {
            Serial.write(myFile.read());
        }
        // close the file:
        myFile.close();
    }
    else {
        // if the file didn't open, print an error:
        Serial.println(F("Unable to open file"));
    }
    */
}