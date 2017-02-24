#include <Mii.h>

#include <MiiFunctions.h>

//Function to restart the
void(* RESTART) (void)=0; //declare reset function @ address 0

/**
Convert a hex char to byte value
*/
uint8_t hexCharToByte(char c) {
  return ((c>='0' && c<='9') ? c-'0' : ((c>='A' && c<='F') ?  c-'A'+10 : ((c>='a' && c<='f') ? c-'a'+10 : 0)));
}

//Read a analog value using a number of tries
int readAnalog(uint8_t pin,uint8_t trys){
  if (trys==0) trys=1;
  long val=0;
  for (uint8_t i=trys;i>0;i--){
    val+=analogRead(pin);
  }
  return val/trys;
}


#ifdef MII_PIN_BATTERY
/**
 * Returns the actual value of analog reference (either DEFAULT, INTERNAL or EXTERNAL).
 * Calls analogRead() to set ADMUX.
 */
uint8_t getAnalogReference() {
  // Makes sure mode has been set in ADMUX by calling analogRead() on method's first call
  analogRead(ANALOG_REFERENCE_DUMMY_PIN);
  return (ADMUX >> 6);
}

/**
 * Replacement method for analogReference(). Makes sure the next call to analogRead() will
 * return the right value. Calls analogRead().
 */
void setAnalogReference(byte mode) {
  analogReference(mode);                   // switch to new reference
  // XXX I don't know why but without these lines the switch from
  // DEFAULT to INTERNAL does not work well.
  analogRead(ANALOG_REFERENCE_DUMMY_PIN);
  delay(5); // wait for 5 ms to let analog settle
}

/**
 * Returns the analog value at #pin# by using the analog reference mode
 * specified by #mode#. Switches back to original mode after.
 */
int analogReadReference(byte pin, byte mode) {
  uint8_t currentReference = getAnalogReference(); // copy current reference
  setAnalogReference(mode);               // safely switch to new reference
  int value = analogRead(pin);            // read value
  setAnalogReference(currentReference);   // switch back to original reference
  return (value);
}

// Function created to obtain chip's actual Vcc voltage value, using internal bandgap reference
// The ability to read processors Vcc voltage and the ability to maintain A/D calibration with changing Vcc
uint16_t getBandGap(void) { // Returns actual value of Vcc (x 100)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
     // For mega boards
     const long InternalReferenceVoltage = 1115L;  // Adjust this value to your boards specific internal BG voltage x1000
        // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc reference
        // MUX4 MUX3 MUX2 MUX1 MUX0  --> 11110 1.1V (VBG)         -Selects channel 30, bandgap voltage, to measure
     ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR)| (0<<MUX5) | (1<<MUX4) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);
  #else
     // For 168/328 boards
     const long InternalReferenceVoltage = 1102L;//1056L;  // Adjust this value to your boards specific internal BG voltage x1000
        // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc external reference
        // MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)         -Selects channel 14, bandgap voltage, to measure
     ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);
  #endif
  delay(50);  // Let mux settle a little to get a more stable A/D conversion
  // Start a conversion
  ADCSRA |= _BV( ADSC );
  // Wait for it to complete
  while( ( (ADCSRA & (1<<ADSC)) != 0 ) );
  // Scale the value
  return (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L; // calculates for straight line value
}

//BATTERY --------------------------------------------------
uint16_t getBattery(uint8_t pin,uint16_t divider) {
  return (map(readAnalog(pin), 0, 1023, 0, getBandGap())*1000L)/divider;
}

uint8_t getBatteryLevel(uint8_t pin,uint16_t batMin,uint16_t batMax,uint16_t divider) {
 return map(getBattery(pin,divider),batMin,batMax,0,11);
}

#endif

/**
Turn the led on or off, showing the color requested
*/
void setLed(uint8_t color){
  #ifdef MII_PIN_LED_RED
   digitalWrite(MII_PIN_LED_RED,(color & LED_RED) ? LOW : HIGH);
   digitalWrite(MII_PIN_LED_GREEN,(color & LED_GREEN) ? LOW : HIGH);
   digitalWrite(MII_PIN_LED_BLUE,(color & LED_BLUE) ? LOW : HIGH);
  #endif
}

//BUZZER --------------------------------------------------
void setBuzzer(uint8_t state){
  #ifdef MII_PIN_BUZZER
  digitalWrite(MII_PIN_BUZZER,state==0 ? LOW : HIGH);
  #endif
}

//Convert an analog trim-resistor into a position (0-15)
uint8_t trimPosition(uint8_t pin){
  uint16_t val=analogRead(pin);
  if (val<500 || val>980) return 0;
  if (val<700) return map(val,500,700,1,(MAX_POSITIONS+1)/2);
  return map(val,690,980,(MAX_POSITIONS+1)/2,MAX_POSITIONS);
}



