/*
 * SRF02.h - library for SRF02

  Copyright (c) Sierk Hoeksma 2011

  The library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  12 Jan 2012 - Initial release
 */

#if defined(ARDUINO) && ARDUINO >= 100
#define printIIC(args) Wire.write((uint8_t)args)
#define readIIC() Wire.read()
#else
#define printIIC(args) Wire.send(args)
#define readIIC() Wire.receive()
#endif

#include "SRF02.h"

#define cmdByte   0x00                            // Command byte for software version
#define lightByte 0x01                            // Byte to read light sensor
#define rangeByte 0x02                            // Byte for start of ranging data

#define SRF02_DELAY 100                            //65ms before returing messure range and sometime to let messaure die away.

SRF02::SRF02(byte address,byte rMode){
  init(address,rMode); //Default
}

SRF02::SRF02(void){
 init(SRF02_ID,SRF02_CM); //Default
}

byte SRF02::realAddress(byte address){            //Convert a spec adress into a i2c adress
 if (address>128) {
   address>>=1; //the address specified in the datasheet is 224 (0xE0) but i2c adressing uses the high 7 bits so it's 112 (0x70)

 } else if (address<16) {
   address=0x70+(2*address);
 }
 return address;
}

void SRF02::init(byte address,byte rMode){
  Wire.begin();
  srfAddress=realAddress(address);
  rangeMode=rMode;
  _delay=SRF02_DELAY;
}

byte SRF02::isPresent() {                       //Return 1 when SRF02 is present
 Wire.beginTransmission(srfAddress);
 printIIC(cmdByte);
 return (Wire.endTransmission() == 0) ? 1 : 0;
}

int SRF02::getRange(boolean block) { //When Block is not set we return directly otherwise -1 will indicate no reading
  if (locked!=0 && locked>millis()) return SRF02_BLOCKED; //Device is still messuring when we did not pass locked time
  if (locked==0) {
    Wire.beginTransmission(srfAddress);             // Start communticating with SRF02
    printIIC(cmdByte);                             // Send Command Byte
    printIIC(rangeMode);                           // Send 0x51 to start a ranging
    if (Wire.endTransmission()!=0) return SRF02_UNAVAILABLE;                                 // Return -3 indicating range not connected
    locked=millis()+_delay;
    if (!block) return SRF02_BLOCKED;
    else  delay(_delay);//Wait the specified delay
  }
  Wire.beginTransmission(srfAddress);             // start communicating with SRFmodule
  printIIC(rangeByte);                           // Call the register for start of ranging data
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)srfAddress,(uint8_t)2);                // Request 2 bytes from SRF module
//  while(Wire.available() < 2);
  locked=0;
  highByte = readIIC();                      // Get high byte
  lowByte = readIIC();                       // Get low byte
  return (highByte << 8) + lowByte;              // Put them together
}

byte SRF02::getLight(){                                   // Function to get light reading
  Wire.beginTransmission(srfAddress);
  printIIC(lightByte);                           // Call register to get light reading
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)srfAddress,(uint8_t)1);                // Request 1 byte
  //while(Wire.available() < 0);
  int lightRead = readIIC();                 // Get light reading
  return lightRead;                              // Returns lightRead
}

byte SRF02::getVersion(){                                    // Function to get software revision
  Wire.beginTransmission(srfAddress);             // Begin communication with the SRF module
  printIIC(cmdByte);                             // Sends the command bit, when this bit is read it returns the software revision
  if (Wire.endTransmission()!=0) return 0xFF;
  Wire.requestFrom((uint8_t)srfAddress,(uint8_t)1);                // Request 1 byte
  //while(Wire.available() < 0);
  return readIIC();                  // Get byte
}

void SRF02::changeId(byte newAddress){            //Set the new adress 0-15
  Wire.beginTransmission(srfAddress);
  printIIC(0xA0);
  printIIC(0xAA);
  printIIC(0xA5);
  printIIC(SRF02_ID + (newAddress*2));
  Wire.endTransmission();
}
