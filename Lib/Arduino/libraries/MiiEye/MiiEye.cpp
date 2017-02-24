#include <MiiEye.h>
//TODO: With new devices add Serial print to ISR. See if number of events start low
MiiEye* MiiEye::_self = 0;

MiiEye::MiiEye(uint8_t selectPin,uint8_t interruptPin,uint8_t testPin,uint8_t oldPin){
  _interruptPin = interruptPin;
  _csPin = selectPin;
  _testPin=testPin;
  _oldPin=oldPin;
}

bool MiiEye::reset(){
  _external=false;
  _armed=false;
  eye_switchTime=0;
  eye_lastLowTime=0;
  eye_switchLast=0;
  _inValidInt=0;
  return true;
}

bool MiiEye::init(bool external,uint32_t timeout){
  // Software reset the device
  reset();
  // Determine the interrupt number that corresponds to the interruptPin
  int interruptNumber = digitalPinToInterrupt(_interruptPin);
  if (interruptNumber == NOT_AN_INTERRUPT) return false;
  setTimeout(timeout);
  _external=external;

  if (!_external && _testPin!=MII_EYE_NO_PIN ) {
       pinMode(_testPin,OUTPUT);
       digitalWrite(_testPin,LOW); //Make sure pin is low
       pinMode(_testPin,INPUT);
       delay(1); //GIVE TIME TO PULL UP;
       _external=digitalRead(_testPin);
  }
  if (!_external && _oldPin!=MII_EYE_NO_PIN) {
      pinMode(_oldPin,INPUT_PULLUP);
      delay(1); //GIVE TIME TO PULL UP;
      _external=!digitalRead(_oldPin);
  }

   pinMode(_csPin,OUTPUT);
   #if MII_VERSION >= 36
     digitalWrite(_csPin,_external ? LOW : HIGH); // --> As off version 3.6 we power the transistor
   #else
     digitalWrite(_csPin,_external ? HIGH : LOW); // --> T3 (RESET) is controlled by IR led
   #endif
   pinMode(_interruptPin, INPUT);
   validate(); //Call this to init the states
   // Enable the interrupt
   attachInterrupt(interruptNumber, validateISR, CHANGE);
  _self=this;
  return true;
}


//IR_Beam_Changed is called every time the state is low or high
bool MiiEye::validate() {
  uint8_t state=digitalRead(_interruptPin);
  if (state==eye_state) {
    return false;
  }
  eye_state=state;
  //Build the timing, if not external remove the trigger delay
  uint32_t time = millis() - (_external ? 0 : MII_IR_TRIGGER_DELAY);

  if (_external) {
    if (state==EXTERNAL_STATE) {
      if (!eye_switchTime && eye_lastLowTime) {
        eye_switchTime=time;
      }
      eye_lastLowTime=0;
    } else { //LOW GATE is closed
      if (!eye_lastLowTime) {
        eye_lastLowTime=time;
      }
    }
    //TODO:WE NEED TO GO HIGH BEFORE NEXT TRIGGER
  } else if (_armed) { //BEAM armed
    if (state!=ARM_STATE) {
      if (!eye_lastLowTime) { //We have switch
        eye_lastLowTime=time;
      }
    } else { //High
      if (eye_lastLowTime) {
       eye_switchTime=eye_lastLowTime;
      }
      eye_lastLowTime=0; //High state needed to reset eye_lastLowTime
    }
  } else { //BEAM unarmed
     if (state!=ARM_STATE) { //Signal droped, no beam
       if (!eye_lastLowTime) {
         eye_lastLowTime=time;
       }
       eye_switchTime=0;
     } else {   //HIGH Signal raised, beam
       if (!eye_switchTime){
          eye_switchTime=time;
       }
       eye_lastLowTime=0;
     }
  }
  return true;
}

void MiiEye::validateISR(){
 //We should test if we are already in loop
 if (_self && !_self->validate())
    //intCounter is updated
    _self->_inValidInt++;
}

bool MiiEye::isActive(){
  return _external || _armed || (!_armed && eye_switchTime);
}

//Get the IR break time
uint32_t MiiEye::getTime(bool peek){
  //Call the eye_beam_changed just to be sure we did not mis a interrupt
  validate();
  uint32_t t= millis();

  if (!_external) { //Only check state on beam
     if (!_armed && eye_switchTime && t>=eye_switchTime+MII_IR_BEAM_ARM_TIMEOUT) {
        eye_switchTime=0;
        eye_lastLowTime=0;
        eye_switchLast=0;
        _armed=true;
        return 0;
     }
     //Check if the bream is to long so we should unarm
     if (eye_lastLowTime && t>=eye_lastLowTime+MII_IR_BEAM_ARM_TIMEOUT && t>=eye_switchLast+MII_IR_BEAM_ARM_TIMEOUT) {
        _armed=false;
        eye_switchTime=0;
        eye_lastLowTime=0;
        eye_switchLast=0;
        return 0;
     }
  }

  //Should we make a real switch
  if ((_external || _armed) && eye_switchTime) {
    if (eye_switchTime>=eye_switchLast+_timeout){
      if (peek) return eye_switchTime;
      eye_switchLast=eye_switchTime;
      eye_switchTime=0;
      return eye_switchLast;
    } else { //Invalid time
       eye_switchTime=0;
    }
  }
  return 0;
}
