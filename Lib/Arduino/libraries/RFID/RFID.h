/*
 * RFID.h - library for Reading a RFID tag using a software serail port
 */
#ifndef RFID_h
#define RFID_h



#include <Arduino.h>
#include <SoftwareSerial.h>

byte xtod(byte c);

class RFID : public  SoftwareSerial {
private:
  // per object data
  int pos;       //index of char number
  byte reading;

public:
  // public methods
  RFID(uint8_t receivePin);
  boolean process();
  operator bool();
  void reset();

  //The read tag, only valid when check is 1
  char tag[13]; //Tag is one longer to keep as string ending
  byte vendor;
  unsigned long id;
};

#endif
