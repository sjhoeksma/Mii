#include <DS1820.h>

int DS1820::read(){
  int HighByte, LowByte, TReading, SignBit, Tc_100;
  OneWireReset(_pin);
  OneWireOutByte(_pin, 0xcc);
  OneWireOutByte(_pin, 0x44); // perform temperature conversion, strong pullup for one sec

  OneWireReset(_pin);
  OneWireOutByte(_pin, 0xcc);
  OneWireOutByte(_pin, 0xbe);

  LowByte = OneWireInByte(_pin);
  HighByte = OneWireInByte(_pin);
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 =(6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
  if (SignBit) Tc_100*=-1;
  return Tc_100-_correction;
}

void DS1820::OneWireReset(int Pin) // reset.  Should improve to act as a presence pulse
{
     digitalWrite(Pin, LOW);
     pinMode(Pin, OUTPUT); // bring low for 500 us
     delayMicroseconds(500);
     pinMode(Pin, INPUT);
     delayMicroseconds(500);
}

void DS1820::OneWireOutByte(int Pin, byte d) // output byte d (least sig bit first).
{
   byte n;

   for(n=8; n!=0; n--)
   {
      if ((d & 0x01) == 1)  // test least sig bit
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(5);
         pinMode(Pin, INPUT);
         delayMicroseconds(60);
      }
      else
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(60);
         pinMode(Pin, INPUT);
      }

      d=d>>1; // now the next bit is in the least sig bit position.
   }

}

byte DS1820::OneWireInByte(int Pin) // read byte, least sig byte first
{
    byte d, n, b;

    for (n=0; n<8; n++)
    {
        digitalWrite(Pin, LOW);
        pinMode(Pin, OUTPUT);
        delayMicroseconds(5);
        pinMode(Pin, INPUT);
        delayMicroseconds(5);
        b = digitalRead(Pin);
        delayMicroseconds(50);
        d = (d >> 1) | (b<<7); // shift d to right and insert b in most sig bit position
    }
    return(d);
}

long altitudeRation = 15384615;
long DS1820::altitudeDiff(int baseTemp,int topTemp) //return height Difference in meters
{
 //In average we have 0.0065C/m (153.84615m/c), base and top in 100
 return ((topTemp>baseTemp ? topTemp-baseTemp : baseTemp-topTemp) * altitudeRation)/100;
}