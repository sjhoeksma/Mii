/*
  ShiftLCD Library - setCursor
 
 Demonstrates the use a 16x2 LCD display.  The ShiftLCD library works with
 all LCD displays that are compatible with the Hitachi HD44780 driver.
 There are many of them out there, and you can usually tell them by the
 16-pin interface.
 
 This sketch prints to all the positions of the LCD using the
 setCursor method:
 
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
 This example originaly by Tom Igoe, Jul 2009
 Example modified for use with ShiftLCD
 Chris Parish, January 12th 2010
 
 */

// include the library code:
#include <SRAM.h>
#include <ShiftLCD.h>

// these constants won't change.  But you can change the size of
// your LCD using them:
const int numRows = 2;
const int numCols = 16;

// initialize the library with the numbers of the interface pins
ShiftLCD lcd(2, 4, 3);

void setup() {
  // set up the LCD's number of rows and columns: 
  lcd.begin(numRows, numCols);
}

void loop() {
  // loop from ASCII 'a' to ASCII 'z':
  for (int thisLetter = 'a'; thisLetter <= 'z'; thisLetter++) {
    // loop over the columns:
    for (int thisCol = 0; thisCol < numRows; thisCol++) {
      // loop over the rows:
      for (int thisRow = 0; thisRow < numCols; thisRow++) {
        // set the cursor position:
        lcd.setCursor(thisRow,thisCol);
        // print the letter:
        lcd.print(thisLetter, BYTE);
        delay(200);
      }
    }
  }
}


