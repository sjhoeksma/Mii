#ifndef LCD_h
#define LCD_h
/**
 * This is an abstraction class enabling to use different LCD drivers
 */
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SRAM.h>

class LCD : public Print {
public:
  LCD() {}
  virtual void clear();
  virtual void home();
  virtual void noDisplay();
  virtual void display();
  virtual void noBlink();
  virtual void blink();
  virtual void noCursor();
  virtual void cursor();
  virtual void scrollDisplayLeft();
  virtual void scrollDisplayRight();
  virtual void leftToRight();
  virtual void rightToLeft();
  virtual void autoscroll();
  virtual void noAutoscroll();
  virtual void backlightOn();
  virtual void backlightOff();
  virtual void createChar(uint8_t,const uint8_t[]);
  virtual void setCursor(uint8_t, uint8_t);
  virtual size_t write(uint8_t);
  void print_P(PGM_P str) {if (inSRAM(str)) print(str); else for (uint8_t c; (c = pgm_read_byte(str)); str++) write(c);}
};

#endif