#ifndef MiiRFDevice_h
#define MiiRFDevice_h
//Extending the MiiRF todo time keeping for clients

#include <Mii.h>
#include <MiiFunctions.h>

#define MII_DEFAULT_START_DELAY 2400
#define MII_PARALLEL_START_DELAY 5000
//We use a count down of 4 seconds
#define MII_COUNT_DOWN_TIME 4000

#include <MiiGenericModem.h>
#if MII_MODEM == 95
  #include <MiiRF95Modem.h>
#elif MII_MODEM == 22
  #include <MiiRF22Modem.h>
#elif  MII_MODEM == 1
  #include <MiiRFClass.h>
#else
 #error Please specify MII_MODEM
#endif

class MiiRFDevice : public MII_MODEM_CLASS {
public:
  MiiRFDevice(uint8_t slaveSelectPin = MII_PIN_RF_CS, uint8_t interruptPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN);
  virtual bool sendTiming(); //Send timing to other devices
  virtual void timingChanged(); //New timing has been set
  virtual void modeChanged(); //Called when mode has changed
  bool addTime(uint32_t time,uint8_t sourceAddress);//Add time and do send timing
  bool setStartDelay(uint32_t value); //Set start delay of start device only
  bool setBoundary(uint32_t minvalue,uint32_t maxvalue); //Set the boundaries for the master
  virtual bool setSessionDate(uint32_t date);
  bool setSession(uint16_t session);
  bool setNextUserId(uint16_t id);
  bool changeUser(changeuser_ptr rec);
  bool setMode(uint8_t mode);
  bool setExtra(uint16_t extra); //Set the extra information
  bool setState(stateRec_t &state); //Set the state of active
  bool swapFinish(); //Swap the finish time to next user
  bool noFinish(); //Reopen the last finish item
  virtual void updateData(){};
  virtual void clearData();
  bool getTimeSync(); //Overwrite TimeSync so that we don't sync during active items.
  bool available(); //Overwrite available so we can do processing
  virtual void countdownGo(){} //Function called on countdown finished


protected:
  uint32_t _unusedTime; //Unused time
  uint8_t  _unusedAddress; //Unused address
  uint32_t _sendTiming; //When we send a time for last time
  timing_t timing;  //Last read timing
  uint16_t _restoreUser; //The user id to restore
  uint32_t _startDelay; //Time needed before new time will be added
  uint32_t _minBoundary; //Min time before accepting
  uint32_t _maxBoundary; //Max time before accepting
  uint8_t  _timingMode;  //The mode timing is
  uint32_t _lastAction;  //The time of a lastAction
  uint32_t _countdown;   //The time to use for a count down
  bool _isChanged; //Indicator that that has changed
  bool internalProcess();
  uint8_t updateActiveCount();
  bool addTiming(uint32_t time,uint8_t sourceAddress);//Add a time without sending timing

};

#endif