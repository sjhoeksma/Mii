// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <Wire.h>
#include <Time.h>              // Basic time keeping
#include <DS1307RTC.h>


void setup () {
    Serial.begin(9600);
    Wire.begin();
//    RTC.begin();

  if (! RTC.isRunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.set(__DATE__, __TIME__);
  }
}

void loop () {
    time_t now = RTC.get();

    Serial.print(year(now), DEC);
    Serial.print('/');
    Serial.print(month(now), DEC);
    Serial.print('/');
    Serial.print(day(now), DEC);
    Serial.print(' ');
    Serial.print(hour(now), DEC);
    Serial.print(':');
    Serial.print(minute(now), DEC);
    Serial.print(':');
    Serial.print(second(now), DEC);
    Serial.println();
    delay(3000);
}