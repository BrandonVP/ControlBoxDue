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
#define ARM1_RX 0x0CA
#define ARM1_M 0xA3
#define ARM1_T 0xA2
#define ARM1_B 0xA1
#define ARM1_CONTROL 0x0A0
#define CHANNEL1 0

// Arm 2 IDs
#define ARM2_RX 0x0CB
#define ARM2_M 0xB3
#define ARM2_T 0xB2
#define ARM2_B 0xB1
#define ARM2_CONTROL 0x0B0
#define CHANNEL2 1

// Prevents physical button doubletap
#define BUTTON_DELAY 200

void pageControl(int page, bool value);
