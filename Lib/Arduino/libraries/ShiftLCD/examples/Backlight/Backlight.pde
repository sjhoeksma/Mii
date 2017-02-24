/*
  ShiftLCD Library - Backlight
 
 Demonstrates the use a 16x2 LCD display.  The ShiftLCD library works with
 all LCD displays that are compatible with the Hitachi HD44780 driver.
 There are many of them out there, and you can usually tell them by the
 16-pin interface.
 
 This sketch displays "Hello, World!" and then flashes the display
 on and off once a second
 
  The circuit:
 
---Shift Register 74HC595---
 * SR Pin 14 to Arduino pin 2
 * SR Pin 12 to Arduino pin 3
 * SR Pin 11 to Arduino pin 4
 * SR Pin  8 to Ground
 * SR Pin 16 to +5v
 * SR Pin 13 to Ground
 * SR Pin 10 to +5v
 -----Shift Reg to LCD--------
 * SR Pin 15 to D7
 * SR Pin 1  to D6
 * SR Pin 2  to D5
 * SR Pin 3  to D4
 * SR Pin 5  to LEDK
 * SR Pin 6  to Enable
 * SR Pin 7  to RS
 -----LCD HD44780-------------
 * Vss to Ground
 * Vdd to +5V
 * Vo  to 10k Wiper
 * R/W to Ground
 * 5v  to +5v
 
 For a more detailed schematic, please see my blog:
 
 http://cjparish.blogspot.com/2010/01/controlling-lcd-display-with-shift.html 
 
 Library modified from the original LiquidCrystal Library
 Example by
 Chris Parish, January 12th 2010
 
 */

// include the library code:
#include <SRAM.h>
#include <ShiftLCD.h>

// initalise the library with the numbers of the interface pins
ShiftLCD lcd(2,4,3);

void setup()
{
  // set up the LCD's number of rows and columns:
  lcd.begin(16,2);
  // Output Hello, World!
  lcd.print("Hello, World!");
}

void loop()
{
  // Turn the backlight on
  lcd.backlightOn();
  delay(1000);
  // Turn the backlight off
  lcd.backlightOff();
  delay(1000);
}
