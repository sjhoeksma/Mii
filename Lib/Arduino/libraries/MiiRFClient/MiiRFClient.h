#ifndef MiiRFClient_h
#define MiiRFClient_h
//Extending the MiiRF todo time keeping for clients

#include <Mii.h>

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

class MiiRFClient : public MII_MODEM_CLASS
 {
public:
 ///Init the based independent of RF device used
  MiiRFClient(uint8_t selectPin = MII_PIN_RF_CS, uint8_t intPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN);

  //Public vars, overwritten every time
   virtual void changed(timeRec_ptr newTime,timeRec_ptr oldTime){}
   virtual void start(timeRec_t &timeRec){}
   virtual void intermediate(timeRec_t &timeRec){}
   virtual void finish(timeRec_t &timeRec){}
   virtual void heartbeat(){}
   bool setSessionDate(uint32_t sessionDate); //Set the sessionDate by sending it to master
   bool setNextUserId(uint16_t  userId); //Set the next user id by sending it to master
   bool setExtra(uint16_t extra); //Set the extra information
   bool setState(uint8_t state); //Set the state of active
   bool setDNF(); //Set active state to DNF
   bool changeUser(changeuser_ptr rec); //Change a user
   bool setStartDelay(uint32_t value);
   bool setBoundary(uint32_t minvalue,uint32_t maxvalue);
   bool setMode(uint8_t mode); //Set the operation mode
   bool swapFinish(); //Swap the last finish
   bool noFinish(); //Remove a invalid finish
   bool isParallelMode(){return timing.mode & MII_MODE_PARALLEL;}
   bool isTimeMode(){return timing.mode & MII_MODE_TIME;}
   void clearData();

protected:
  //internalProcess allows you to do internal processing of commands during available checks
  bool internalProcess();
  virtual void updateData();
  timing_t timing;  //the read timing
  timing_t ntiming; //Last processed timing
  timeRec_t finishTime[MII_MAX_FINISH_TIMES]; //Record of the finishes
  timeRec_t activeTime;   //The active time record
  uint8_t   activeCount; //The active count
  uint16_t  nextUser;    //The next user to start
  uint32_t  _countdown;   //Countdown time


};

#endif