#ifndef MiiEye_h
#define MiiEye_h
//MiiEye is implementation the eye
#include <Mii.h>

/* MiiChrono Beam Logic
MiiChrono Base frequency is around 3200Hz in this periode 9 pulse have to be received
otherwise the trigger will go off. So the time it will take to break the beam is
3200/9=355Hz=0,0028ms
To ensure that there are enough opportunities to reset the trigger the beam is sending
pulse at a rate of 2400hz resulting in that we have to lose 6 pulses before triggering
2400hz/355hz=6,7 pulses
Result of this all is that we will correct the time with -3ms because of trigger delay
we take 3ms because the is a changes we missed 1 tick (0,00028ms)

Beam 555 38khz   C1=680pf R1=2.2K R2=22K (potmeter) triggerout = 0
         2.4khz  C1=
*/

//----------------------------------------------------------------------
//Constants for the IR BEAM arming is 3.5 seconds
//Default switch timeout is 750 ms
//----------------------------------------------------------------------
#define MII_IR_BEAM_ARM_TIMEOUT 3500
#define MII_IR_SWITCH_TIMEOUT   750
#define MII_IR_TRIGGER_DELAY    3

#define MII_EYE_NO_PIN 0xFF

class MiiEye {
public:
  MiiEye(uint8_t selectPin=MII_PIN_EYE_CS,uint8_t interruptPin=MII_PIN_EYE_IRQ,uint8_t testPin=MII_EYE_NO_PIN,uint8_t oldPin=MII_EYE_NO_PIN);
  bool init(bool external=false,uint32_t timeout=MII_IR_SWITCH_TIMEOUT);        //Attached the interrupt
  bool isActive();      //Return true when the beam is active
  uint32_t getTime(bool peek=false); //Return the time of last switch (0 when no switch)
  bool validate();    //Validate the input port
  bool reset();       //Reset the stuff and detach intterupt
  void setTimeout(uint32_t timeout){_timeout=timeout;} //Set timeout between switches
  bool available(){return getTime(true)!=0;}//Return true if there is a time available
  bool isExternal(){return _external;}
  bool isArmed(){return (_external || _armed) ;}
  bool isReady(){return _self!=0;}
  volatile uint32_t _inValidInt; //Counts the number of invalid interrupt calls


protected:
  //Local varabiles
  volatile uint32_t eye_switchTime; //Time when last switched
  volatile uint32_t eye_lastLowTime; //Time when went low
  volatile uint8_t  eye_state;       //The eye state
  uint32_t eye_switchLast; //Time when we accepted last switch
  bool _external;  //Is system internal controlled or by Eye
  bool _armed;     //Is the system armed
  uint8_t _interruptPin;
  uint8_t _csPin;
  uint8_t _testPin;
  uint32_t _timeout;
  uint8_t _oldPin;
private:
 /// Low level interrupt service routine for MiiEye connected to interrupt 0
  static void validateISR();
  static MiiEye* _self;

};

#endif