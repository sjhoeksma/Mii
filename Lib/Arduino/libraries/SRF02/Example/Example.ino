#include <Wire.h>
#include <SRF02.h>

SRF02 sensor;

void setup()
{
  Serial.begin(9600);          // start serial communication at 9600bps
}

void loop()
{
  Serial.print("Version:");
  Serial.println(sensor.getVersion());  
  Serial.print("Block Range Read :");
  Serial.println(sensor.getRange());  
  Serial.print("Unblocked Range Read :");
  int range,i=0;
  while ((range=sensor.getRange(false))==SRF02_BLOCKED){i++;}
  Serial.print(i);
  Serial.print('-');
  Serial.println(range);
  delay(1000);                  // wait a bit since people have to read the output :)
}
