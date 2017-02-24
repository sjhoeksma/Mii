#ifndef MiiFunctions_h
#define MiiFunctions_h
//Common functions for Mii programs

#include <Mii.h>

 //declare reset function @ address 0
extern void(* RESTART) (void);

//Convert a char to hex value
uint8_t hexCharToByte(char c);

//Get the analogReference
uint8_t getAnalogReference();

/**
 * Replacement method for analogReference(). Makes sure the next call to analogRead() will
 * return the right value. Calls analogRead().
 */
//Set the analogReference we would like to use,  DEFAULT to INTERNAL does not work well.
void setAnalogReference(byte mode);

/**
 * Returns the analog value at #pin# by using the analog reference mode
 * specified by #mode#. Switches back to original mode after.
 */
int analogReadReference(byte pin, byte mode);

int readAnalog(uint8_t pin,uint8_t trys=4);

//----------------------------------------------------------------------
//Constants for BATTERY use them as between
//----------------------------------------------------------------------
#define  BATTERY_EMPTY  2
#define  BATTERY_LOW    4
#define  BATTERY_MEDIUM 6
#define  BATTERY_FULL   8

#define  MAX_POSITIONS  15

#ifdef MII_PIN_BATTERY
// Function created to obtain chip's actual Vcc voltage value, using internal bandgap reference
// The ability to read processors Vcc voltage and the ability to maintain A/D calibration with changing Vcc
uint16_t getBandGap(void);

//Read a battery state, divider is the ratio used on the pin 500 is half
uint16_t  getBattery(uint8_t pin=MII_PIN_BATTERY,uint16_t divider=500);
//Convert the battery state into a value from 10=Full 1=Empty
uint8_t getBatteryLevel(uint8_t pin=MII_PIN_BATTERY,uint16_t batMin=350,uint16_t batMax=420,uint16_t divider=500);
#endif
//Turn the led on or off, showing the color requested
void setLed(uint8_t color);

//Turn the buffer on or off
void setBuzzer(uint8_t state);

//Convert an analog trim-resistor into a position (1-15)
uint8_t trimPosition(uint8_t pin);
//Read a analog value

#endif