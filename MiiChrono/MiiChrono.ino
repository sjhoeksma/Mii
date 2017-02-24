/* Test Client/sever to test communication between two MiiChronoV3.6 devices
   Validate settings, channel,syncwords Ok
   Validate time sync    Ok
   Validate ack system   Ok
   Validate Yield        Ok
   Validate Bluetooth    Ok
   Validate MiiEye       Ok 
   Validate Battery      Ok
   Validate retry system Ok  
   Validate Collision    Ok
   Validate trimPosition Ok
   Validate parallelMode OK
   Validate swapFinish   Ok
   Validate noFinish     Ok
   Validate autoModem
   
   
TODO:
   if send ack to your self push it on to the stack
   Set power as 1-9 and use mapping for real value
   When restart sometimes the start hangs ?
   Support a way to print end results after the run ended (support remote print)
   
   Remove all bluetooth actions and simplify it. 1 Command to Host(Containing message for RF) and 2 a Command to retrieve status info
   Create box below LCD which could contain 2 beam and 2 devices and if possible 5 legs

   Indicator for Power not correct
   Use GSM to send commands to update properties, master sends to all registered clients and waits for OK --> DEVICEIID CODE
   
   When reading Eprom keep the original baud, so we can init the modem correrctly.
   
CODE
    Make the app in Ionic
    Fix Back in ionic
    Use _intCounter in Eye to validate if we are bouncing the interrupt
    When using Master Finish and device is not used for some time we have to show finish again.  Old Code ?

    
BOARD 4.1
    + Add pullup ok 10k to Switch pin. Allowes to use hall without pullup  
    * Add Meassure point for freq
    * Add Resistor to change freq on detail
    * Add step up, allowing to use one battery
    * Create board for Wifi in slot GSM
    * Create board for Bluetooth in slot GSM
 
    Test Wifi + Long Range sender on Step up 5v from two pack battery
   
DONE:
* Remove livespan: Switch was not accepted because livespan of prevAddress had expired in addTime
* Added Printer support
* StartDelay is now managed by Master and start
* Usage of new mode configuration
* Added noFinish
* Fixed swapFinish
* Fixed Bluetooth modem
* State changes works now on start instead of id
* Eye works for version 3 and 5
* Range limitation for version S4432 20Db still not stable
* Fixed firmware size in config
*/

//Set to address of device if we should configure the device MII_DEV_MASTER +/- x allow compiler to over rule
#define MII_ADDRESS MII_DEV_MASTER
#define MII_ADDRESS MII_DEV_START
//#define MII_ADDRESS MII_DEV_FINISH
//#define MII_ADDRESS MII_DEV_PRINTER

//Disable the buzzer for quite testing
#define MII_DISABLE_BUZZER 0

//Set to 1 if we have Bluetooth
#define MII_BLUETOOTH 1

//Set to 1 if we have a beam or 2 if we a combined gate 3 only beam
#define MII_IR_BEAM 1

//Set the default power level, to maximum level
#define MII_TXPOW MII_TXPOW_23DBM

//Default a device has now clock
#define MII_CLOCK 0

//Default a device is not connected to printer, but if set to 1 it will connect to the printer
#define MII_PRINTER 0

//Default a device is not connected to a ESP device
#define MII_ESP 0

#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
//Include the libraries
#include <Mii.h>
//Check if we have configured the correct device
#if MII_VERSION<10 || MII_VERSION>=1000
#error Please set the correct MII_VERSION in <Mii.h>
#endif

//Check if we are printer, disable beam and disable bluetooth
#if MII_ADDRESS == MII_DEV_PRINTER
#define MII_PRINTER 1
#define MII_IR_BEAM 0
#endif

#if MII_PRINTER
#include <MiiPrinter.h>
#define MII_BLUETOOTH 0
#endif


#include <MiiConfig.h>
#include <MiiFunctions.h>
#include <OnOff.h>
#include <SPI.h>
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
#include <MiiRFDevice.h>

#if MII_BLUETOOTH
#include <MiiBluetooth.h>
#else
  #define LOG_DEBUG(str)
  #define LOG_INFO(str)
  #define LOG_WARN(str)
  #define LOG_FAIL(str)
  #define LOG_DEBUG_T(time,str)
  #define LOG_INFO_T(time,str)
  #define LOG_WARN_T(time,str)
  #define LOG_FAIL_T(time,str)
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2)
  #define LOG_WARN_T2(time,str,str2)
  #define LOG_FAIL_T2(time,str,str2)
#endif
#include <EEPROM.h>            //The Basic EEPRom lib
#include <EEPROMAnything.h>    //Read write blocks from eeprom 

//Disable clock does not work it makes SDA behavior strange (HIGH)
#if MII_CLOCK
#include <DS3232RTC.h>            
#include <Wire.h>
#endif

#if MII_ESP
#include <MiiESPCom.h>
MiiEspCom Esp(Serial);
#endif

uint8_t buf[MII_RF_MESSAGE_LEN];

#if MII_PRINTER
class MiiLocalPrinter : public MiiPrinter { public: 
   MiiLocalPrinter() : MiiPrinter(Serial) {}
   void onConnected(){
    MiiPrinter::onConnected();
    doOnOff(&setLed,LED_BLUE,175,175,3); //Bluetooth Connect
    print(F("Connected:"));print(MiiConfig.group%1000);print('.');print(MiiConfig.channel);print('.');println(MiiConfig.address);
    if (year()>2000)  { print(F("Date:"));date(now());println(); }
    print(F("Device "));print(F("Switch"));tab(17);print(F("Nr   "));println(F("Time"));
    line(30);
   }

   void onError(){
    MiiPrinter::onError();
    doOnOff(&setLed,LED_RED,175,175,3); //Bluetooth Connect error   
   }
};
MiiLocalPrinter Printer;
#endif


#ifdef MII_PIN_GATE
 void setGate(uint8_t state){
  digitalWrite(MII_PIN_GATE,state);
}
#endif

//Overwrite some Device function so we can work the ligth and keep track of last msg 
class MiiRFHost : public MiiRFDevice { public:
 MiiRFHost(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
  : MiiRFDevice(selectPin,intPin,sdnPin) {
  }

  #if MII_CLOCK
   bool setSessionDate(uint32_t date){
    bool ret =  MiiRFDevice::setSessionDate(date);
    if (ret) {
      uint32_t t = date-(time()/1000);
      RTC.set(t); 
      setTime(t);
    }
   }
  #endif

  bool syncTime(uint8_t address=MII_BROADCAST_ADDRESS){
     doOnOff(&setLed,LED_RED,175); //Failed hart beat client
     return MiiRFDevice::syncTime(address);
  }
  
  //Called when the system time has been changed
  void timingChanged(){
    if (isMaster()){
      doOnOff(&setLed,closestAddress() ? LED_HEART : LED_RED,250); //Flash red if no clients
    } else {
      doOnOff(&setLed,time() ? LED_HEART : LED_RED,250); //Flash red if no time sync
    }
    _lastTiming=millis();
    //On changed data on master side short next hardbeat 
    if (isMaster() && _isChanged) {
       _lastTiming=millis() - (MII_HEARTBEAT_INTERVAL/2);
    } else  _lastTiming=millis();  
    MiiRFDevice::timingChanged();
    #if MII_BLUETOOTH
      Bluetooth.send(time(),MII_CMD_DATA,(uint8_t*)&timing,sizeof(timing_t));
    #endif
    LOG_DEBUG(F("Timing Changed"));
   #if MII_PRINTER
    if (Printer.isConnected()){ 
      //Check if session date has changes, if so change the internal time
      if (timing.sessionDate && timing.sessionDate!=ntiming.sessionDate) {
        setTime(timing.sessionDate+time());
      }
      //Now Check if data has changed
      if (timing.times[0].start+1000<ntiming.times[0].start) { //New start time, clear the data
        Printer.println("Restart Timing");
      } 
      for (int i=0;i<MAX_TIMES;i++){
        int j=0;
        if (timing.times[i].start>ntiming.times[0].start){ //We have a new data
          //We need to keep track of closed starts
          Printer.print(F("Start"));
          Printer.tab(8);
          Printer.print(timing.times[i].start);
          Printer.tab(17);
          Printer.println(timing.times[i].user);
          if (timing.times[i].state>=MII_STATE_FINISH){
            j=MAX_TIMES;
            goto printFinish;
          }
        } else {
           for (j=0;j<MAX_TIMES;j++){
             //We see same time as start and user equal
            if (timing.times[i].start==ntiming.times[j].start &&
             timing.times[i].user!=ntiming.times[j].user){ //user  has changed
              Printer.print(F("Change User "));
              Printer.print(ntiming.times[i].user);
              Printer.print(F(" to "));
              Printer.println(timing.times[i].user);
            }
           //We see same time as start and user equal
           if (timing.times[i].start==ntiming.times[j].start &&
             (timing.times[i].time!=ntiming.times[j].time ||
             timing.times[i].state!=ntiming.times[j].state)
            ){ //Time or state has changed
 printFinish:
              if (timing.times[i].state<MII_STATE_DNF) {
                Printer.print(                   
                   timing.times[i].state==0 ? F("Reopen") :
                   timing.times[i].state<MII_STATE_FINISH ? F("Inter") :
                   timing.times[i].state==MII_STATE_FINISH ? F("Finish") :
                   timing.times[i].state==MII_STATE_FINISH_PREV ? F("Fin L") : F("Fin R") );
                if (timing.times[i].state!=MII_STATE_FINISH_PREV && timing.times[i].state!=MII_STATE_FINISH_LAST) {   
                  Printer.tab(8);
                  Printer.print(timing.times[i].start+timing.times[i].time);
                }
                Printer.tab(17);
                Printer.print(timing.times[i].user);
                if (timing.times[i].state!=0) {
                  if (timing.times[i].state==MII_STATE_FINISH_PREV || timing.times[i].state==MII_STATE_FINISH_LAST) {
                    Printer.tab(22);
                    Printer.print('+');
                    Printer.time(timing.times[i].time,false);
                  } else {
                    Printer.tab(23);
                    Printer.time(timing.times[i].time);
                  }
                }
                Printer.println();
              } else { //State >=DNF
                Printer.print(F("Manual"));
                Printer.tab(17);
                Printer.print(timing.times[i].user);
                Printer.tab(23);
                Printer.println(timing.times[i].state==MII_STATE_DNF ? F("DNF") :
                   timing.times[i].state<=MII_STATE_DSQ_R ? F("DSQ") :
                   timing.times[i].state<=MII_STATE_DNS_R ? F("DNS") :
                   timing.times[i].state==MII_STATE_REMOVED ? F("DEL") :
                   F("INV"));           
              }
            }
         }
        }
      }
    }
    ntiming=timing;
   #endif
  }
 
  #if MII_PRINTER
  switchRec_t switches[MII_MAX_GATES];
  timing_t ntiming; //Last processed timing
  //Catch the swith Command
  void logSwitch(uint8_t id,uint32_t time){
    if (!Printer.isConnected()) return;   
    uint8_t i=0;
    for (;i<MII_MAX_GATES && switches[i].id!=0;i++){
       if (id==switches[i].id) {
          goto updateSwitch;
        }
    }
    if (i<MII_MAX_GATES && switches[i].id==0) { //We need to add
 updateSwitch:       
        if (time!=switches[i].time) {
          //Just Print the switch
          Printer.print(id);
          Printer.tab(8);
          Printer.println(time);    
          //Store the switch time so we don't print double times
          switches[i].time=time;
          switches[i].id=id;
        }
      }
  }
  
  void afterSend(const uint8_t* data=0, uint8_t len=0){
    if (_txHeaderId==MII_CMD_SWITCH) {
       logSwitch(_txHeaderFrom,*(uint32_t*)data);
    }
    MiiRFDevice::afterSend();
  }
  #endif

   #if MII_ESP || MII_PRINTER
   bool beforeProcess(){ 
   #if MII_ESP
     uint8_t len; 
     len=MII_SIZEOF_timing_t;
     recv((uint8_t*) &smallbuf[0],&len,true);
     Esp.send((uint8_t)_rxHeaderId,(uint8_t*)&smallbuf[0],len);
   #endif
   #if MII_PRINTER
   if (_rxHeaderId==MII_CMD_SWITCH) {    
        uint32_t time;
        uint8_t len=sizeof(uint32_t);
        if (recv((uint8_t*) &time,&len,true)) {
          logSwitch(_rxHeaderFrom,time);  
        }
    }
    #endif
    return MiiRFDevice::beforeProcess();
  }
  #endif

    //Pull up the GATE pin if defined
  #ifdef MII_PIN_GATE
  void countdownGo(){
   setGate(LOW);
   doOnOff(&setGate,HIGH,5000); //Set the gate high after 5 seconds
  }
  #endif
     
  //Called by main loop, todo some processing   
  void doProcess(){
     #if MII_ESP
      if (Esp.available()) {
        if (Esp.isAction()){
          uint8_t len= MII_SIZEOF_timing_t;
          Esp.read((uint8_t*)smallbuf,len);
          Esp.send(sendAckCmd(Esp.getCmd(),(uint8_t*)smallbuf,len) ? MII_ESP_CMD_OK : MII_ESP_CMD_FAIL,0,0,false);
        } else Esp.clear();
      }
    #endif
    if (isMaster() && _lastTiming+MII_HEARTBEAT_INTERVAL<millis()) {
         if (!sendTiming()) {
            doOnOff(&setLed,LED_RED,175); //Failed hart beat
         }
    }
    if (!isMaster() && (_lastTiming+(MII_HEARTBEAT_INTERVAL*4))<millis()){
      _lastTiming=millis();
       doOnOff(&setLed,LED_RED,175); //Failed hart beat client
    } 
  }
  
  uint32_t _lastTiming;
   
};

//Init the classes we will use
MiiRFHost rf(MII_PIN_RF_CS,MII_PIN_RF_IRQ,MII_PIN_RF_SDN);

#if MII_IR_BEAM
#include <MiiEye.h>
 #if MII_IR_BEAM == 3
  MiiEye eye(MII_PIN_EYE_CS,MII_PIN_EYE_IRQ);
 #elif MII_IR_BEAM == 2
  MiiEye eye(MII_PIN_EYE_CS,MII_PIN_EYE_IRQ,MII_PIN_EYE_T1,MII_PIN_EYE_T2);
 #else
  MiiEye eye(MII_PIN_EYE_CS,MII_PIN_EYE_IRQ,MII_PIN_EYE_T1);
 #endif 
#endif


//Setup all ports and init all classes and variables
void setup() {  
  
  //Init led Pins
  pinMode(MII_PIN_LED_RED,OUTPUT);
  pinMode(MII_PIN_LED_GREEN,OUTPUT);
  pinMode(MII_PIN_LED_BLUE,OUTPUT);
  setLed(LED_OFF);
  
  //Init pin for Buzzer and make sure it is off
  pinMode(MII_PIN_BUZZER,OUTPUT);
  setBuzzer(LOW);
  #if MII_DISABLE_BUZZER 
    pinMode(MII_PIN_BUZZER,INPUT); //Disable buzzer by making pin input
  #endif
  
  #if MII_CLOCK
    //RTC.set(1431961440L);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
  #endif
  
  //Battery Check
  uint8_t battery = getBatteryLevel();
  setLed(battery<=BATTERY_LOW ? LED_RED : (battery<=BATTERY_MEDIUM ? LED_BLUE : LED_GREEN));

  if (readConfig(MII_DEVICE_ID,MII_ADDRESS,MII_TXPOW,MII_FIRMWARE)){
    //Whe have read new config file  
    #if MII_BLUETOOTH
      Bluetooth.begin(MII_DEVICE_ID,MII_ADDRESS);
    #endif  
    LOG_INFO(F("Config EEPROM"));
  } else {  
   #if MII_BLUETOOTH
     Bluetooth.begin();
     LOG_INFO(F("Normal Start"));
   #endif
  }
  //Keep the battery led on for at least 1 seconds + modem startup
  delay(1000);
    
  //We set the mode
  uint8_t mode = MII_MODE_TIME;
  
  /* We have disable this code, we should only use one of them as control function
   #if MII_VERSION >= 38
     uint8_t pos;
     //Group pin will control channel and group so you can link devices from diffrent types 
     if (pos=trimPosition(MII_PIN_GROUP)) { 
        MiiConfig.group=pos*MAX_POSITIONS;
        MiiConfig.channel=pos*3;
     } 
     if (pos=trimPosition(MII_PIN_FREQ))  { //Set powerlevel instead of freq
          MiiConfig.power=map(pos,1,MAX_POSITIONS,MII_TXPOW_1DBM,MII_TXPOW_20DBM);
     }
     //Check if we should change the id, or even go to parrallel mode
     if (pos=trimPosition(MII_PIN_ID)) {
         if (pos>12){ //MAX ID WHE CAN SET 240
           MiiConfig.address=MII_DEV_MASTER;
           mode = MII_MODE_PARALLEL;
         } else MiiConfig.address=pos*20;
     }
  #endif
  */  
   
   if (!rf.init(MiiConfig.address,MiiConfig.group,MiiConfig.channel,MiiConfig.power,MiiConfig.address==MII_DEV_MASTER)) {
 
    LOG_WARN(F("Failed RF"));
    doOnOff(&setLed,LED_RED,175,175,100);
    delay(5000);
    RESTART();
  }
  rf.setMode(mode);
  
  
  #if MII_CLOCK
    if (rf.isMaster()){
      rf.setSessionDate(now());
    }
  #endif
  
  #if MII_BLUETOOTH >= 2
      sprintf((char*)buf,"Device: %d.%d.%d %d",MiiConfig.group%1000,MiiConfig.channel,MiiConfig.address,mode);
      Bluetooth.log(0,3,(char*)buf);
  #endif 
 
  setLed(LED_OFF);
    
  #if MII_IR_BEAM
    if (!eye.init(false)) {
      LOG_WARN(F("Failed EYE"));
      RESTART();
    } 

   if (eye.isExternal()) {LOG_WARN(F("EXTERNAL EYE"));}
  #endif

  #if MII_PRINTER
    Serial.begin(MII_PRINTER_BAUD);
    Printer.begin("MSP-100",1234);
  #endif

  #if MII_ESP
    Esp.begin();
  #endif

  //Pull up the GATE pin if defined
  #ifdef MII_PIN_GATE
   pinMode(MII_PIN_GATE,OUTPUT);
   setGate(HIGH);
  #endif
}


//In the RF Library yield is called when we are in waiting loops. 
//Make sure we call validate so we don't miss a event
void yield(){
  #if MII_IR_BEAM
  if (MiiConfig.address < MII_DEV_CLIENT && eye.isReady()){ //Only run this process when eye is ready.
    eye.validate(); //Ensure the eye is validate during idle time
    //Check if the beam is armed
    if (!eye.isExternal() && !eye.isArmed()) { //If we are not external and not arm show it
        if (eye.isActive()){
          doOnOff(&setLed,LED_GREEN,1000);
          if (MiiConfig.armSound) delOnOff(&setBuzzer); //Sound off so we can aim beam
        } else {
          doOnOff(&setLed,LED_RED,1000);
          if (MiiConfig.armSound) doOnOff(&setBuzzer,HIGH,1000); //Sound beep short so we can aim beam
        }
    } 
  }
  #endif
  processOnOff(); //Run the background process for OnOff
}

void loop(){
  //Check the IR beam
  #if MII_IR_BEAM
   if (MiiConfig.address < MII_DEV_CLIENT ) {
    uint32_t irTime=eye.getTime();
    if (irTime!=0) {
     bool ack=false; 
     doOnOff(&setLed,LED_SWITCH,425); //First long flash
     if (rf.addTime(rf.time(irTime),MiiConfig.address)) {
       doOnOff(&setLed,LED_SWITCH,350); //Long flash when accepted       
       doOnOff(&setBuzzer,HIGH,350);
       LOG_INFO(F("SWITCH TRG OK"));
     } else {
       doOnOff(&setLed,LED_RED,350); //We failed the switch
       LOG_FAIL(F("SWITCH TRG FAILED"));
     }
   } 
  }  
  #endif
  
  //Process any rf data
  if (rf.available()){
    uint8_t len=sizeof(buf);
    bool read=false;
    switch (rf.headerId()){
      #if MII_BLUETOOTH
      case MII_CMD_LOG: {
        if (read=rf.recv(buf,&len)) {
          buf[len]='\0';
          Bluetooth.log(0,(char*)buf);
        }   
      } break;
      #endif
      
     case MII_CMD_GET_CONFIG: {
       if (rf.headerTo()!=MII_BROADCAST_ADDRESS)
          rf.sendCmd(MII_CMD_SET_CONFIG,(uint8_t*)&MiiConfig,sizeof(miiConfig_t),rf.headerFrom());
      } break;
   
    } //End Of Switch
    //Check if we should remove the data
    if (!read) rf.recv();
  } 
  
  
  //Do the bluetooth processing
  #if MII_BLUETOOTH > 0
     uint8_t c = Bluetooth.available();
     switch (c) {
       case 0: break; //No command
        case MII_CMD_SERIALID: { //Return the serial id and our firmware version
         Bluetooth.send(rf.time(),MII_CMD_SERIALID,(uint8_t*)&MiiConfig.id,sizeof(uint32_t)); 
         //SerialId request will also send FIRMWARE back to Device
        }
        
       case MII_CMD_FIRMWARE: {  
         Bluetooth.send(rf.time(),MII_CMD_FIRMWARE,(uint8_t*)&MiiConfig.firmware,sizeof(uint16_t));
        }break;
      
       case MII_CMD_SET_MODE:{ 
          uint8_t mode;
          if (Bluetooth.read(&mode,sizeof(mode))) rf.setMode(mode);
        }break;
     
       case MII_CMD_CLEAR: {
          if (rf.isMaster()) rf.clearData();
          else rf.sendAckCmd(MII_CMD_CLEAR,(uint8_t*)NULL,0);
       }break;

       case MII_CMD_NO_FINISH : {
        rf.noFinish();
       } break;
       case MII_CMD_SWAP_FINISH : {
         rf.swapFinish();
       }break;
    
        case MII_CMD_SET_DATE: {
          uint32_t date;
          //Correct the date (in sec) only if allready running
          if (rf.time() && Bluetooth.read((uint8_t*)&date,sizeof(date))) 
            rf.setSessionDate(date);
        }break;  
   /* //Parts not used comment out save space    
       case MII_CMD_GET_CONFIG: {
         Bluetooth.send(rf.time(),MII_CMD_SET_CONFIG,(uint8_t*)&MiiConfig,sizeof(miiConfig_t));
       }break;  
       
       case MII_CMD_SET_CONFIG: { //Set the config
         if (Bluetooth.read((uint8_t*)buf,sizeof(miiConfig_t))) {
           writeConfig((miiConfig_ptr)buf,7); //Dont write id(4),firmware(2),address(1)
           RESTART();
         }
       }break;
*/
/*
      case MII_CMD_SET_BOUNDARY: {
         if (Bluetooth.read((uint8_t*)buf,sizeof(boundaryRec_t))) 
          rf.setBoundary(((boundaryRec_ptr)buf)->minBoundary,((boundaryRec_ptr)buf)->maxBoundary);
       }break;
       
       case MII_CMD_SET_DELAY:{ 
          uint32_t startDelay;
          if (Bluetooth.read((uint8_t*)&startDelay,sizeof(startDelay))) rf.setStartDelay(startDelay);
       }break;
   */    
       case MII_CMD_SET_STATE: {
         if (Bluetooth.read((uint8_t*)buf,sizeof(stateRec_t))) { 
          rf.setState(*(stateRec_ptr)buf);
         }
       }break;

       case MII_CMD_SET_USER: {
          uint16_t nextUser;
          if (Bluetooth.read((uint8_t*)&nextUser,sizeof(nextUser))) {
            rf.setNextUserId(nextUser);
          }
        }break;
        
        case MII_CMD_SET_SESSION: {
          uint16_t session; 
          if (Bluetooth.read((uint8_t*)&session,sizeof(session))) 
              rf.setSession(session);
          }break;
          
         default: {
          LOG_DEBUG(F("UNKNOWN CMD"));
         }break; 
         
     }
     //Clear the command
     Bluetooth.clear(); 
  #endif
  
  //Do the processing of the rf
  rf.doProcess();
  
  //Do the printer processing
  #if MII_PRINTER
    Printer.doProcess();
  #endif

  
  //Do the pocessing of the on off switching and beam checking
  yield();
}


