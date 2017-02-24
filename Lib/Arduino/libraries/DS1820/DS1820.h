#ifndef ds1820_h
#define ds1820_h
#include <Arduino.h>

//Class to read temperature from DS18200
class DS1820 {
public:
 DS1820(int Pin,int Correction=0) {_pin=Pin;_correction=Correction;};
 int read(); //Read the temp, 1850 is 18.5 deg
 static long altitudeDiff(int baseTemp,int topTemp); //return height Difference in meters
 static void OneWireReset(int Pin);
 static void OneWireOutByte(int Pin, byte d);
 static byte OneWireInByte(int Pin);
protected:
 int _pin;
 int _correction;
};

extern long altitudeRation;

#endif