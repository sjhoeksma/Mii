/*
 * RFID.h - library for reading a RFID using a software serial port

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
#include <RFID.h>

#define RFID_BEGIN 0x02
#define RFID_END   0x03


byte xtod(byte c) {
  return ((c>='0' && c<='9') ? c-'0' : ((c>='A' && c<='F') ?  c-'A'+10 : ((c>='a' && c<='f') ? c-'a'+10 : 0)));
}
/**
* We Just use the recievePin, transmitpin of SotwareSerial is off
*/
RFID::RFID(uint8_t receivePin)  : SoftwareSerial(receivePin,0xFF,false) {
  begin(9600);
  reset();
}

RFID::operator bool() {
  if (reading<2) process();
  boolean ret = reading==2;
  if (reading>1) reading=0; //Reset reading
  return ret; //Return true if we have a valid read
}

boolean RFID::process(){
  //Convert Tag into id and vendor and validate the checksum
  //(62H)XOR(E3H)XOR(08H)XOR(6CH)XOR(EDH)=08H

 while (available()) {
   byte readByte = read();
    if(readByte == RFID_BEGIN){
      reading = 1; //begining of tag
      pos=0;
    } else if(readByte == RFID_END) {
      byte check,value;
      id=0;
      check=vendor=(xtod(tag[0])<<4) + xtod(tag[1]);
      for (pos=2;pos<10;pos+=2){
        id<<=8;
        id+=(value=(xtod(tag[pos])<<4) + xtod(tag[pos+1]));
        check^=value;
      }
      reading=(check==(xtod(tag[10])<<4) + xtod(tag[11])) ? 2 : 3;
      return true;
    } else if (reading==1 && readByte>13 && pos<12) {
      tag[pos++]=readByte;
    }
 }
 return false;
}

void RFID::reset(){
  byte b;
  while (available() && (b=read()) && b!=RFID_END); //Read until readbuffer is empty or we found RFID_END
  pos=0;
  reading=0;
}

