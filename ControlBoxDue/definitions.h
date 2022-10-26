#pragma once
// Global LCD theme color variables
#define themeBackground 0xFFFF // White
#define menuBtnText 0xFFFF // White
#define menuBtnBorder 0x0000 // Black
#define menuBtnColor 0xFC00 // Orange
#define menuBackground 0xC618 //Silver

// For the draw shape functions
#define LEFT 1
#define CENTER 2
#define RIGHT 3

// CAN Bus Message
#define CRC_BYTE                0x00
#define COMMAND_BYTE            0x01

#define SEND_AXIS_POSITIONS     0x01
#define RESET_AXIS_POSITION     0x02
#define SET_LOWER_AXIS_POSITION 0x03
#define SET_UPPER_AXIS_POSITION 0x04
#define MOVE_GRIP               0x0A
#define SET_WAIT_TIMER          0x0B
#define EXECUTE_PROGRAM         0x0C

// Arm 1 IDs
#define CONTROL1_RX 0x1C3 // Direct communicate to controller
#define ARM1_POSITION   0x1A0 // Current axis position broadcast ID
#define ARM1_PROGRAM    0x1A1 // Programmed movements ID
#define ARM1_MANUAL     0x1A2 // Manual movements ID
#define ARM1_CONTROL    0x1A3 // Control commands
#define CHANNEL1 0

// Arm 2 IDs
#define CONTROL2_RX 0x2C3 // Direct communicate to controller
#define ARM2_POSITION   0x2A0 // Current axis position broadcast ID
#define ARM2_PROGRAM    0x2A1 // Programmed movements ID
#define ARM2_MANUAL     0x2A2 // Manual movements ID
#define ARM2_CONTROL    0x2A3 // Control commands
#define CHANNEL2 1

// Bitmap
#define PIXEL_BUFFER 20 // What pixel?

#define PHYSICAL_BUTTON_DELAY 200 //Needs better description
#define DEG "deg" // Why?
#define REFRESH_RATE 400 // Refresh what?!

#define X_PAGE_START 127 // page start for what?

// Used for converting keypad input to appropriate hex place
const PROGMEM uint32_t hexTable[8] = { 1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 };
const PROGMEM String pDir = "PROGRAMS";
const PROGMEM String version = "Version - 1.1.0";