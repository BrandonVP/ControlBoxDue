// For the draw shape functions
#define LEFT 1
#define CENTER 2
#define RIGHT 3

// ** RX Command List ** //

// These are fixed bytes that can not be used for anything else
#define COMMAND_BYTE            0x01
#define SUB_COMMAND_BYTE        0x05
#define CRC_BYTE                0x07 // For CONTROL and MANUAL

// List of commands for the COMMAND_BYTE
#define SEND_AXIS_POSITIONS     0x61
#define RESET_AXIS_POSITION     0x62
#define HOME_AXIS_POSITION      0x63

#define GRIP_BYTE               0x06
#define MOVE_GRIP               0x6A
#define HOLD_GRIP               0x00
#define OPEN_GRIP               0x01
#define SHUT_GRIP               0x11

#define SET_WAIT_TIMER          0x6B
#define SET_MIN_BYTE            0x02
#define SET_SEC_BYTE            0x03
#define SET_MS_BYTE             0x04

#define EXECUTE_PROGRAM         0x1E // Execute Steps
#define STOP_PROGRAM            0x0E // Stop Steps
#define ACCELERATION_BYTE       0x02
#define SPEED_BYTE              0x03
#define LOOP_BYTE               0x04

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

// Used for converting keypad input to appropriate hex place
extern const PROGMEM uint32_t hexTable[8];
const PROGMEM String pDir = "PROGRAMS";
const PROGMEM String version = "Version 2.0.0";