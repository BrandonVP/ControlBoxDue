// For the draw shape functions
#define LEFT 1
#define CENTER 2
#define RIGHT 3

// Global LCD theme color variables
#define themeBackground 0xFFFF // White
#define menuBtnText 0xFFFF // White
#define menuBtnBorder 0x0000 // Black
#define menuBtnColor 0xFC00 // Orange
#define menuBackground 0xC618 //Silver

// ** RX Command List ** //

    // These are fixed bytes that can not be used for anything else
#define COMMAND_BYTE            0x00
#define SUB_COMMAND_BYTE        0x04
#define CRC_BYTE                0x07 // For CONTROL and MANUAL

// List of commands for the COMMAND_BYTE
#define SEND_AXIS_POSITIONS     0x61
#define RESET_AXIS_POSITION     0x62
#define HOME_AXIS_POSITION      0x63

#define MOVE_GRIP_BYTE          0x05
#define MOVE_GRIP               0x6A
#define HOLD_GRIP               0x00
#define OPEN_GRIP               0x01
#define SHUT_GRIP               0x11

#define SET_WAIT_TIMER          0x6B
#define SET_WAIT_MIN_BYTE       0x01
#define SET_WAIT_SEC_BYTE       0x02
#define SET_WAIT_MS_BYTE        0x03

#define EXECUTE_PROGRAM         0x1E // Execute Steps
#define STOP_PROGRAM            0x0E // Stop Steps
#define ACCELERATION_BYTE       0x01
#define SPEED_BYTE              0x02
#define LOOP_BYTE               0x03

#define CONFIRMATION_BYTE       0x01
#define CONFIRMATION            0x1C // Confirm message received
#define NEG_CONFIRMATION        0x0C // Confirm message not received or failed CRC

// ** END ** //

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