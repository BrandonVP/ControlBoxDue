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

// Arm 1 IDs
constexpr auto ARM1_RX = 0x0C1;
constexpr auto ARM1_MANUAL = 0x0A3;
constexpr auto ARM1_UPPER = 0x0A2;
constexpr auto ARM1_LOWER = 0x0A1;
constexpr auto ARM1_CONTROL = 0x0A0;
#define CHANNEL1 0

// Arm 2 IDs
constexpr auto ARM2_RX = 0x0C2;
constexpr auto ARM2_MANUAL = 0x0B3;
constexpr auto ARM2_UPPER = 0x0B2;
constexpr auto ARM2_LOWER = 0x0B1;
constexpr auto ARM2_CONTROL = 0x0B0;
#define CHANNEL2 1

// Bitmap
#define PIXEL_BUFFER 20

#define PHYSICAL_BUTTON_DELAY 200
#define DEG "deg"
#define REFRESH_RATE 400

#define X_PAGE_START 127

void pageControl();

// Used for converting keypad input to appropriate hex place
const PROGMEM uint32_t hexTable[8] = { 1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 };
const PROGMEM String pDir = "PROGRAMS";
const PROGMEM String version = "Version - 1.1.0";