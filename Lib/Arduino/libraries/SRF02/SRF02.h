/*
 * SRF02.h - library for SRF02
 */
#ifndef _SRF02_h
#define _SRF02_h

#include <Wire.h>
#include <Arduino.h>

#define SRF02_ID  0xE0                           // Default Address of the SRF02
#define SRF02_INCHES 0x50
#define SRF02_CM     0x51
#define SRF02_MSEC   0x52

#define SRF02_UNAVAILABLE -3
#define SRF02_BLOCKED -1

// library interface description
class SRF02 {
  // user-accessible "public" interface
  public:
    SRF02(void);                                   //Init the SRF02 default cm and adress 0
    SRF02(byte address,byte rMode);            //Init the SRF02 with a other adress and the mode
    int getRange(boolean block=true);          //Read the range when block is set we will wait for response otherwise -1 is return until rangescan  has been completed
    byte getLight();                            //Get the light value of the SRF02
    byte getVersion();                        //Get Software version of SRF02 when 0xFF than SRF is busy with range scan.
    void changeId(byte newAddress);            //Set the new adress 0-15 of the SRF02
    byte isPresent();                          //Return 1 when SRF02 is present
    byte realAddress(byte address);            //Convert a spec adress into a i2c adress
    void init(byte address,byte rMode);
    void setDelay(byte b){_delay=b;}           //Overwrite the default delay between request


  private:
   byte highByte;                             // Stores high byte from ranging
   byte lowByte;
   byte srfAddress;
   byte rangeMode;
   unsigned long locked;
   byte _delay;

};

#endif