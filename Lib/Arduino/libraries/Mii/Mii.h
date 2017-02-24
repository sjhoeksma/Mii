#ifndef Mii_h
#define Mii_h
// This file contains all constants and definitions used between difference Mii Classes
#include <Arduino.h>

/*
//The hardware version we are using is set in the Mii.h include file please change there
DeviceID Name
2015001  Ski Team Midden Nederland
2015002  SKi Racing Heemskerk
2015006  Ski Racing Heemskerk 2
2016001  Ski Racing Heemskerk 3 Version 5.0 5.1

*/
#ifndef MII_DEVICE_ID
#define MII_DEVICE_ID  2016001
//#define MII_DEVICE_ID  2015006
#endif

//The Firmware version of this software
#ifndef MII_FIRMWARE
#define MII_FIRMWARE 103
#endif

//----------------------------------------------------------------------
//Specify for which version we are building
//----------------------------------------------------------------------
#ifndef MII_VERSION
//#define MII_VERSION 30
//#define MII_VERSION 32
//#define MII_VERSION 34
//#define MII_VERSION 36
//#define MII_VERSION 38
//#define MII_VERSION 50
#define MII_VERSION 51
//ScoreBoard
//#define MII_VERSION 1010
//#define MII_VERSION 1011
//#define MII_VERSION 1012
//#define MII_VERSION 1021
//Controller
//#define MII_VERSION 2010
//Gate
//#define MII_VERSION 3000
#endif

//Set the default behavior off the WATCH DOG
#define MII_WATCHDOG 0

//Set MIIRF_SERIAL to 1 if you need serial debugging, 2 if time sync is needed
#define MIIRF_SERIAL 0

//Interval used to send heartbeats
#define MII_HEARTBEAT_INTERVAL 2500

/* European lookup table for 433 Mhz, Because of bug we should not uses 433.00 */
#define MII_RF_CHANNEL_BASE 433.05
#define MII_RF_CHANNEL_WIDTH 0.05
#define MII_RF_CHANNEL_COUNT 20
//Range uf kHhz
#define MII_RF_CHANNEL_RANGE 1.5


//Time allowed to be my clock of from session date (2 min)
#define MAX_SESSIONDATE_DIFF 120

//Set SPI_ISOLATE to 1 if you use multiple SPI clients
#define SPI_ISOLATE 1

// The message length
#define MII_RF_MESSAGE_LEN 55

//We support maximum 2 cached messages, setting to 0 will disable and save 500bytes
#define MII_MAX_MSG_CACHE 2

//We store by default 5 finish times
#define MII_MAX_FINISH_TIMES  5

//----------------------------------------------------------------------
//Device Id Ranges
//----------------------------------------------------------------------
//Count down Device
#define MII_DEV_COUNT_DOWN 1
//Master device setting
#define MII_DEV_MASTER 100
//Default start device setting
#define MII_DEV_START  80
//Default finish device setting
#define MII_DEV_FINISH 120

//Min Client device settings
#define MII_DEV_CLIENT 200
//Client for Controller
#define MII_DEV_CONTROLLER  230
//Client for scoreboard Count down
#define MII_DEV_SCOREBOARD_COUNT_DOWN  245
//Client for scoreboard
#define MII_DEV_SCOREBOARD  249
//Client device but for MiiBeacon
#define MII_DEV_BEACON 250
//Hand held Display
#define MII_DEV_DISPLAY    252
//Gate unlock for parallel
#define MII_DEV_GATE       253
//Client device but for logging
#define MII_DEV_PRINTER    254

//Constant for a undefined pin
#define MII_NO_PIN        0xFF
//----------------------------------------------------------------------
//Here are the pin definitions by version
//Version 2.0, Code needs to be convert to work
//----------------------------------------------------------------------
#if MII_VERSION == 20
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM 1
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_RF_CS      7
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_IRQ_CS A0
#define MII_PIN_LED_GREEN  A1
#define MII_PIN_LED_BLUE   A2
#define MII_PIN_LED_RED    A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_BATTERY    A7
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version
//Version 3.0
//----------------------------------------------------------------------
#elif MII_VERSION == 30 ||  MII_VERSION == 31
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM 22
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LCD_VCC    5
#define MII_PIN_LCD_LIGHT  6
#define MII_PIN_RF_CS      7
#define MII_PIN_LCD_DC     8
#define MII_PIN_LCD_CS     9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_LCD_DIN    11
#define MII_PIN_MISO       12
#define MII_PIN_LCD_RST    12
#define MII_PIN_SCK        13
#define MII_PIN_LCD_SCK    13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_LED_GREEN  A1
#define MII_PIN_LED_BLUE   A2
#define MII_PIN_LED_RED    A3
#define MII_PIN_SDA        A4
#define MII_PIN_ANALOG_RX  A4
#define MII_PIN_SCL        A5
#define MII_PIN_ANALOG_TX  A5
#define MII_PIN_JOYSTICK   A6
#define MII_PIN_BATTERY    A7
#define MII_PIN_RF_SDN     0xFF
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version
//Version 3.2
//No Bluetooth and no joystick
//----------------------------------------------------------------------
#elif MII_VERSION == 32
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM 22
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LED_GREEN  5
#define MII_PIN_LED_RED    6
#define MII_PIN_RF_CS      7
#define MII_PIN_LED_BLUE   9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_BATTERY    A7
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version
//Version 3.4 - 3.6
//----------------------------------------------------------------------
#elif MII_VERSION >= 34 && MII_VERSION <=36
#define MII_MODEM 22
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LED_GREEN  5
#define MII_PIN_LED_RED    6
#define MII_PIN_RF_CS      7
#define MII_PIN_LED_BLUE   9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_GROUP      A2
#define MII_PIN_FREQ       A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_BLUETOOTH  A6
#define MII_PIN_BATTERY    A7
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version
//Version 3.8, bluetooth pin replaced by ID pin
//----------------------------------------------------------------------
#elif MII_VERSION >= 38 && MII_VERSION < 40
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM  22
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LED_GREEN  5
#define MII_PIN_LED_RED    6
#define MII_PIN_RF_CS      7
#define MII_PIN_LED_BLUE   9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_GROUP      A2
#define MII_PIN_FREQ       A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_ID         A6
#define MII_PIN_BATTERY    A7
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version, extention board
//Version 4.0
//----------------------------------------------------------------------
#elif MII_VERSION >= 40 && MII_VERSION < 50
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM 1
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LED_GREEN  5
#define MII_PIN_LED_RED    6
#define MII_PIN_RF_CS      7
#define MII_PIN_SW2        8
#define MII_PIN_LED_BLUE   9
#define MII_PIN_KEY        10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_NETLED     A2
#define MII_PIN_CTRL       A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_POWLED     A6
#define MII_PIN_BATTERY    A7
#define ARM_STATE 		   HIGH
#define EXTERNAL_STATE     HIGH

//----------------------------------------------------------------------
//Here are the pin definitions by version, extention board
//Version 5.0 & 5.1 bleutooth only connected to 5.1
//----------------------------------------------------------------------
#elif MII_VERSION >= 50 && MII_VERSION < 1000
//Loading the correct modem; 1= OldRF22, 95 = LiRo and 22 = RF22/RF23
#define MII_MODEM 95
#define MII_PIN_TX         0
#define MII_PIN_RX         1
#define MII_PIN_EYE_IRQ    2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_BUZZER     4
#define MII_PIN_LED_GREEN  5
#define MII_PIN_LED_RED    6
#define MII_PIN_RF_CS      7
#define MII_PIN_SW2        8
#define MII_PIN_LED_BLUE   9
#define MII_PIN_UCHPD      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_EYE_CS     A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_BT_RX      A2
#define MII_PIN_BT_TX      A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_CTRL       A6
#define MII_PIN_BATTERY    A7
#define MII_PIN_EYE_T1     MII_PIN_SW2
#define ARM_STATE 		   LOW
#define EXTERNAL_STATE     HIGH
//----------------------------------------------------------------------
//Here are the pin definitions by version of ScoreBoard 1000 - 1999
//Version 1.0 is version without screws and bluetooth
//----------------------------------------------------------------------
#elif MII_VERSION >= 1000 && MII_VERSION < 1011
#define MII_MODEM 22
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_SW         2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_LCD_A      4
#define MII_PIN_LCD_B      5
#define MII_PIN_LCD_C      6
#define MII_PIN_LCD_D      7
#define MII_PIN_LCD_R1     8
#define MII_PIN_LCD_CLK    9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_RF_CS      A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_LCD_LT     A2
#define MII_PIN_LCD_OE     A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_BLED       A6
#define MII_PIN_BATTERY    A7
#define MII_PIN_GATE       2


//----------------------------------------------------------------------
//Here are the pin definitions by version of ScoreBoard 1000 - 1999
//Version 1.2 is version with screws and no bluetooth and time, keyboard
//----------------------------------------------------------------------
#elif MII_VERSION >= 1011 && MII_VERSION < 1020
#define MII_MODEM 22
#define MII_PIN_LCD_LT     0
#define MII_PIN_LCD_OE     1
#define MII_PIN_SW         2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_LCD_A      4
#define MII_PIN_LCD_B      5
#define MII_PIN_LCD_C      6
#define MII_PIN_LCD_D      7
#define MII_PIN_LCD_R1     8
#define MII_PIN_LCD_CLK    9
#define MII_PIN_SD_CS      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_RF_CS      A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_GROUP      A2
#define MII_PIN_FREQ       A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_ID         A6
#define MII_PIN_KEYBOARD   A7
#define MII_PIN_GATE       2

//----------------------------------------------------------------------
//Here are the pin definitions by version of ScoreBoard 1000 - 1999
//Version 2.0 is hardware broken and should never be used
//Version 2.1 has besides ESP a bluetooth and keyboard switch
//----------------------------------------------------------------------
#elif MII_VERSION >= 1021 && MII_VERSION < 2000
#define MII_MODEM 95
#define MII_PIN_BT_TX      0
#define MII_PIN_BT_RX      1
#define MII_PIN_BTX        2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_LCD_A      4
#define MII_PIN_LCD_B      5
#define MII_PIN_LCD_C      6
#define MII_PIN_LCD_D      7
#define MII_PIN_LCD_R1     8
#define MII_PIN_LCD_CLK    9
#define MII_PIN_UCHPD      10
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_RF_CS      A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_LCD_LT     A2
#define MII_PIN_LCD_OE     A3
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_CTRL       A6
#define MII_PIN_KEYBOARD   A6
#define MII_PIN_BRX        A7
//#define MII_PIN_GATE       0

//----------------------------------------------------------------------
//Here are the pin definitions by version of Controller 2000 - 2999
//----------------------------------------------------------------------
#elif MII_VERSION >= 2000 && MII_VERSION < 3000
#define MII_MODEM 22
//We should disable message caching, because of memory constraint
#define MII_MAX_MSG_CACHE  0
//We need more the 4 Finish Times
#define MII_MAX_FINISH_TIMES 15
//There are no other SPI clients
#define SPI_ISOLATE 0
#define MII_PIN_SW         2
#define MII_PIN_RF_IRQ     3
#define MII_PIN_OLED_RST   4
#define MII_PIN_RF_CS      7
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_KEYBOARD   A0
#define MII_PIN_RF_SDN     A1
#define MII_PIN_JOYSTICK   A2
#define MII_PIN_SDA        A4
#define MII_PIN_SCL        A5
#define MII_PIN_BATTERY    A7

//----------------------------------------------------------------------
//Here are the pin definitions by version of MiiGate 3000 - 3999
//----------------------------------------------------------------------
#elif MII_VERSION >= 3000 && MII_VERSION < 4000
#define MII_MODEM 22
#define MII_MAX_MSG_CACHE  5
#define MII_MAX_FINISH_TIMES 4
//There are no other SPI clients
#define SPI_ISOLATE        0
#define MII_PIN_GATE       0
#define MII_PIN_RF_IRQ     3
#define MII_PIN_RF_SDN     4
#define MII_PIN_RF_CS      7
#define MII_PIN_MOSI       11
#define MII_PIN_MISO       12
#define MII_PIN_SCK        13
#define MII_PIN_LED        A0
#define MII_PIN_BATTERY    A7

//----------------------------------------------------------------------
//Missing version pin definitions missing
//----------------------------------------------------------------------
#else  //LAST else will give error
#error No Valid MII_VERSION specified, PIN definitions not available
#endif

//Incase we did not define the eye test pin's
#ifndef MII_PIN_EYE_T1
#define MII_PIN_EYE_T1 MII_PIN_SDA
#endif
#ifndef MII_PIN_EYE_T2
#define MII_PIN_EYE_T2 MII_PIN_SCL
#endif

//Set MII_ROUNDIG to 5 if time should be rounded up
#define MII_ROUNDING 0

//----------------------------------------------------------------------
//Some common macro stuff
//----------------------------------------------------------------------
#define QUOTEVAL(arg) #arg
#define QUOTE(arg) QUOTEVAL(arg)

//The pin we will use for analog reference
#define ANALOG_REFERENCE_DUMMY_PIN 4

//----------------------------------------------------------------------
//Common color functions for LED
//----------------------------------------------------------------------
#define LED_OFF    0x00
#define LED_RED    1
#define LED_GREEN  2
#define LED_BLUE   4
#define LED_YELLOW LED_RED | LED_GREEN
#define LED_PINK   LED_RED | LED_BLUE
#define LED_LIGHT_BLUE LED_GREEN | LED_BLUE
#define LED_WHITE  LED_RED | LED_GREEN | LED_BLUE
//Named LED colors
#define LED_ERROR  LED_RED
#define LED_HEART  LED_GREEN
#define LED_RESET  LED_GREEN | LED_BLUE
#define LED_ARM    LED_RED
#define LED_SWITCH LED_BLUE

//----------------------------------------------------------------------
//Default we are in time (start - finish) mode
//----------------------------------------------------------------------
#define MII_MODE_TIME         0x80
#define MII_MODE_PARALLEL     0x40
//Should we follow session list
#define MII_MODE_SESSIONLIST  0x20

//----------------------------------------------------------------------
//Constants containing the configuration stored in EEPROM
//----------------------------------------------------------------------
//The location of the config record in the EEPROM
#define MII_CONFIGLOCATION 0
//This config structure is read and stored in EEPROM

//Base structure for  MiiConfig
typedef struct {
    uint32_t  id;                     //The serial id of the device, fixed for allways
    uint16_t  firmware;               //The firmware loaded, fixed for program version
    uint8_t   address;                //The device address used
    uint16_t  group;                  //The id used to group devices together  { 0x2d, 0xd4 }
    uint8_t   channel;                //The device channel to use
    uint8_t   power;                  //The power level to used during transmission
    uint16_t  timeout;                //The min time between two switches
    boolean   armSound;              //Should we play arm sound, to help guiding arming.
    uint32_t  startDelay;            //Delay in ms
    uint32_t  minBoundary;           //The minimum time we accept it as valid, 0 controll is off
    uint32_t  maxBoundary;           //The maximum time we accept as valid time, 0 controll is off
} miiConfig_t, *miiConfig_ptr;

//----------------------------------------------------------------------
//The structures, for communication and time keeping
//----------------------------------------------------------------------
//The number of max active times in device, maxTimes should be even to support parallel
#define MAX_TIMES 4


//Struct for storing collision data
typedef struct {
  uint8_t headerId;
  uint8_t headerFrom;
  uint8_t headerTo;
  uint8_t headerFlags;
  uint8_t data[MII_RF_MESSAGE_LEN];
  uint8_t length;
  uint32_t rxTime;
} msgRec_t, *msgRec_ptr;

//We support maximum 8 address connected to, setting 0 will disable address caching
#define MII_MAX_ADDRESS   8
typedef struct {
  uint8_t  id; //The address we have seen
  uint32_t seen;    //When did we see this address and when
  uint8_t  rssi;    //What was the RSSI of this address
} addressRec_t, *addressRec_ptr;

//We support maximum 6 active gates in the set up
#define MII_MAX_GATES   6
typedef struct {
  uint8_t  id;      //The got switch for
  uint32_t time;    //When did we see this address and when
} switchRec_t, *switchRec_ptr;


//Struct for storing single time
typedef struct {
  uint16_t user;
  uint8_t  state;
  uint32_t start;
  uint32_t time;
} timeRec_t, *timeRec_ptr;


typedef struct {
  uint32_t start;
  uint8_t state;
} stateRec_t, *stateRec_ptr;

//Size of data should be at least 54 in MII_RF  (4x11+2+4+2+2+1=54)
#define MII_SIZEOF_timing_t 54
typedef struct {
  uint16_t  sessionId;
  uint32_t  sessionDate;
  uint16_t  nextUser;
  uint16_t  extra; //For parallel extra can contain the second user id
  uint8_t   mode;  //First Nibble of mode will contain the number of active items
  timeRec_t times[MAX_TIMES];
} timing_t, *timing_ptr;

//Structure for sending a boundary
typedef struct {
  uint32_t minBoundary;
  uint32_t maxBoundary;
} boundaryRec_t, *boundaryRec_ptr;

//States used in communication
#define MII_STATE_FINISH   200
#define MII_STATE_FINISH_PREV 201
#define MII_STATE_FINISH_LAST 202
#define MII_STATE_DNF      205
#define MII_STATE_DSQ      210
#define MII_STATE_DSQ_L    211
#define MII_STATE_DSQ_R    212
#define MII_STATE_DNS      215
#define MII_STATE_DNS_L    216
#define MII_STATE_DNS_R    217
#define MII_STATE_REMOVED  254
#define MII_STATE_ERR      255

//Structure used for a rename
typedef struct {
  uint8_t from;
  uint32_t time;
  uint16_t user;
} changeuser_t, *changeuser_ptr;



#if (ARDUINO >= 155 && !defined(RH_PLATFORM_ATTINY)) || (TEENSYDUINO && defined(__MK20DX128__))
 #define YIELD yield();
#else
 #define YIELD
#endif

//Bit functions clearBit and setBit
//Clear Bit
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
//Set Bit
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
//Verify bit
#ifndef vbi
#define vbi(sfr,bit) (_SFR_BYTE(sfr) & _BV(bit))
#endif


//----------------------------------------------------------------------
//Commands supported
//----------------------------------------------------------------------
#define MII_CMD_SWITCH       1
#define MII_CMD_SET_DELAY    2
#define MII_CMD_DATA         3
#define MII_CMD_SET_USER     4
#define MII_CMD_LOG          5
#define MII_CMD_CLEAR        6
#define MII_CMD_SET_MODE     7
#define MII_CMD_SET_CONFIG   8
#define MII_CMD_SET_STATE    9
#define MII_CMD_SET_BOUNDARY 10
#define MII_CMD_SET_EXTRA    11
#define MII_CMD_SET_SESSION  12
#define MII_CMD_GET_CONFIG   13
#define MII_CMD_SET_DATE     14
#define MII_CMD_SERIALID     15
#define MII_CMD_SWITCH_ACK   16
#define MII_CMD_SWAP_FINISH  17
#define MII_CMD_CHANGE_USER  18
#define MII_CMD_FIRMWARE     19
#define MII_CMD_NO_FINISH    20
#define MII_CMD_COUNT_DOWN   21

#endif