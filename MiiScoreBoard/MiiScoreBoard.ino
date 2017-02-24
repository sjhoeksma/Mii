/* TODO
As of version 2.1 we should 
-support printer
-ESP-01
-EEpprom to store data

Parallel mode 
0. Aansluitingen over GSM pins
1. Start (keyboard (CTRL) or External Command) count down over buzzer and external speaker (BRX)
2. On start down unlock the gates and start clock

If key on analog use digtal read instead of analog.
on beam add audio plug as charger

*/

//The Reduction of the ID
#define MII_DEV 0
 
//Default a device is not connected to printer, but if set to 1 it will connect to the printer
#define MII_PRINTER 0

//Enable or disable ESP
#define MII_ESP 0

//Do we have the internal clock enabled
#define MII_CLOCK 0

//Include the libraries
#include <Mii.h>
//Check if we have configured the correct device
#if MII_VERSION<1000 || MII_VERSION>=2000
#error Please set the correct MII_VERSION in <Mii.h>
#endif

#include <MiiConfig.h>
#include <MiiFunctions.h>
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
#include <MiiRFClient.h>
#include <LEDMatrix.h>
#include <EEPROM.h>            //The Basic EEPRom lib
#include <EEPROMAnything.h>    //Read write blocks from eeprom 
#include <Time.h>
#include <TimeLib.h>
#if MII_CLOCK
#include <DS3232RTC.h>         
#include <Wire.h>        
#endif

#if MII_ESP
#include <MiiESPCom.h>
MiiEspCom Esp(Serial);
#endif


//Default powerlevel must be after Mii.h
#define MII_POWER MII_TXPOW_17DBM
//Width and Height of display
#define WIDTH   64
#define HEIGHT  16
//We wait for 8.5 seconds before returning to next time
#define FREEZE_DELAY 8500
//We refresh display every .5 sec
#define REFRESH 500
//Time in ms when clock returns to time (3 min)
#define RELEASE_TIME 180000L

// Display Buffer 128 = 64 * 16 / 8
uint8_t displaybuf[WIDTH *HEIGHT / 8];
uint8_t drawbuf[WIDTH *HEIGHT / 8];
//Small buffer used for intermediate stuff like date
char smallbuf[MII_SIZEOF_timing_t];

// LEDMatrix    (a, b, c, d, oe, r1, stb, clk);
LEDMatrix lcd(MII_PIN_LCD_A,MII_PIN_LCD_B, MII_PIN_LCD_C, MII_PIN_LCD_D, MII_PIN_LCD_OE, 
              MII_PIN_LCD_R1, MII_PIN_LCD_LT, MII_PIN_LCD_CLK );

#if MII_PRINTER
#include <MiiPrinter.h>
#include <SoftwareSerial.h>
//SoftwareSerial sSerial(MII_PIN_BRX ,MII_PIN_BTX); 
SoftwareSerial sSerial(1,2); //Temp fix for version 2.1 (A7,2)
class MiiLocalPrinter : public MiiPrinter { public: 
   MiiLocalPrinter() : MiiPrinter(sSerial) {}
   void onConnected(){
    MiiPrinter::onConnected();
    print(F("Connected:"));print(MiiConfig.group%1000);print('.');print(MiiConfig.channel);print('.');println(MiiConfig.address);
    if (year()>2000)  { print(F("Date:"));date(now());println(); }
    print(F("Device "));print(F("Switch"));tab(17);print(F("Nr   "));println(F("Time"));
    line(30);
   }
   void onError(){
  _step=0; //Failure
   }

   void doProcess() {
  if (_step>=BT_READY) return;
  if (_wait>millis() && _step!=17) {
    return; //We still need to wait before we can process next command
  }
  int cmd=0;

  switch (_step) {
     case 0 :
    // serial.println(F("AT"));
      serial.println(F("AT"));
      goto processWaitExit;
/*
     case  0 :
       serial.println(F("AT+DISC"));  //Disconnect, can throw error
       goto processWaitExit;
     case 1:
       clearBuf(); ///Ignore error of not connected
       _step++;
       break;
*/
     case 2 :
      serial.println(F("AT+RMAAD"));  //will release the module from any previous PAIR.
      goto processWaitExit;
     case 4 :
      serial.println(F("AT+ROLE=1"));  //Set mode to Master
      goto processWaitExit;
     case 6 :
      serial.println(F("AT+RESET"));  //After changing role, reset is required
      goto processWaitExit;
     case 8:
      serial.println(F("AT+CMODE=1"));  //Allow connection to any address
      goto processWaitExit;
     case  10:
      serial.print(F("AT+INQM=0,")); serial.print(BT_SEARCH_COUNT);serial.print(',');serial.println(BT_SEARCH_TIME);  //Inquire mode - Standard, stop after 5 devices found or after 5 seconds
      goto processWaitExit;
     case  12:
      serial.print(F("AT+PSWD="));serial.println(_pinCode);  //Set the pinCode
      goto processWaitExit;
     case  14 :
      serial.println(F("AT+INIT"));  //Start serial Port Profile (SPP) ( If Error(17) returned - ignore as profile already loaded)
      goto processWaitExit;
     case 15  :
       clearBuf(); ///Ignore error of 14
       _step++;
       break;

     case 16 :
       clearBuf();
       serial.println(F("AT+INQ"));  //Start searching for devices, we will fall through to 17
       _step++;
       _wait=millis()+(BT_SEARCH_TIME+5)*1000; //We will wait the search time and 5 second before erroring out
       _listPos=0;
       break;

     case 17 :
       if (_wait<millis()) {
        if (_listPos>0) { //Should not happend but just to be sure.
          _step++;
        } else {
           onError(); //OOPS We timed out, go back to begining
        }
       } else {
         while (serial.available()) {
           cmd=serial.read();
           //Fill the address table--> +INQ:2:72:D2224,3E0104,FFBC <part1>:<part2>:<part3> , type, signal
           if (cmd=='+') {
              //SKIP INQ:
              uint8_t pos = 4;
              while (pos>0) { if (serial.available()) {cmd=serial.read();pos--;}}
              pos=0;
              while (pos<sizeof(btaddress_t) && cmd!=','){
                if (serial.available()) {
                  cmd=serial.read();
                  _list[_listPos].address[pos++]=cmd==',' ? '\0' : cmd==':' ? ',' : cmd;
                }
              }
              _listPos++;
              //Remove till end of line
              while (serial.read()!='\r'){}
           } else if (cmd=='O') {
             while (!serial.available() &&  _wait<millis()){}
             if (serial.read()=='K') {
              clearBuf();
              _step++;
              return;
             }
           }
         }
       }
       break;

     case 18:
      //We found a bluetooth device and stored them in the list
      //Now loop and query name for each found device and see if we have match
      clearBuf();
      if  (_listPos>0) {
        _listPos--;
        serial.print(F("AT+RNAME?"));serial.println(_list[_listPos].address);
        _wait=millis()+4000;
        _step++;
      } else {
        _step=16; //No Data search again
      }
      break;

     case 19:
        while (serial.available()) {
          cmd=serial.read();
          //Fill the name--> +RNAME:NAME and match it
          if (cmd=='+') {
              //SKIP RNAME:
              uint8_t pos = 6;
              while (pos>0) { if (serial.available()) {cmd=serial.read();pos--;}}
              pos=0;
              bool match=false;
              while (serial.available() && (cmd=serial.read())!='\r') {
                if (_name[pos++]==cmd) {
                  match=true;
                } else match=false;
              }
             if (cmd=='\r' && match) { //Now we have found match do a link
               _step++;
               return;
             }
          }
        }
        clearBuf();
        _step=18;
     break;

     case 20:
      clearBuf();
      serial.print(F("AT+LINK="));serial.println(_list[_listPos].address);
      _wait=millis()+4000;
      _step++;
      break;

     case 21:
       //Check for Failure
       while (serial.available() && (cmd=serial.read())!='F') {}
       if (cmd=='F' && serial.read()=='A') { //FAILURE
         onError(); //Error state
         return;
       }
       _step=BT_READY;
       onConnected();

     case BT_READY: //This is the final step we are connected
      break;


    case 1: case 3: case 5: case 7: case 9: case 11: case 13:
       while (serial.available() && (cmd=serial.read())!='O') {
         lcd.print(cmd); 
       }
       if (cmd=='O' && serial.read()=='K') {
         _step++;
       } else {
         lcd.print('F');
         onError(); //Error state
       }
       break;
  }
  return;
processWaitExit:
  _step++;
  _wait=millis()+MII_BLUETOOTH_CMD_WAIT;
}


};
MiiLocalPrinter Printer;
#endif 

#ifdef MII_PIN_KEYBOARD
uint32_t timeKey=0;
int keyBase=0;
uint8_t getKey() {
  int val=analogRead(MII_PIN_KEYBOARD);
  bool keyVal=val > keyBase+30  || val < keyBase-30;
  uint8_t ret = 0;
  if (keyVal && timeKey==0) timeKey=millis();
  if (timeKey && !keyVal) {
    timeKey=millis()-timeKey;
    if (timeKey>10000) ret = 3;
    else if (timeKey>1200 && timeKey<5000) ret = 2;
    else if (timeKey>100 && timeKey<1000) ret = 1;
    timeKey=0;
  }
  return ret;
}
#endif

//Overwrite some Device function so we can work the ligth and keep track of last msg 
class MiiRFHost : public MiiRFClient { public:
 MiiRFHost(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
  : MiiRFClient(selectPin,intPin,sdnPin) {
   lastRec.state=0xff;
  //  setRetries(5); //We want to try long on our command  
 
 }
  
  uint32_t nextRefresh;
  void heartbeat(){
   activityDot=!activityDot; //Inveret dot, to indicate whe have had data
   lcd.drawPixel(WIDTH-1,0,activityDot ? 0 : 1,true); 
   #if MII_CLOCK
   if (_timeDiff) { //Only update if we are synced with master
     if (timing.sessionDate==0) { //Send hour clock as session date
       setSessionDate(now());
     } else {
       uint32_t t = timing.sessionDate+(time()/1000);
       if (!(now()>t-MAX_SESSIONDATE_DIFF && now()<t+MAX_SESSIONDATE_DIFF)) { //Abs was not working
         RTC.set(t); setTime(t);
       }
     }
   }
   #endif
  }

  void finish(timeRec_t &timeRec){
    draw(timeRec,true);
    nextRefresh=((time()+FREEZE_DELAY)/REFRESH)*REFRESH; //Freeze the finish time
  }
  
  //The time sync has changed for a refresh
  void timeChanged(){
    nextRefresh=0;
  }
  
  void intermediate(timeRec_t &timeRec){
    draw(timeRec,true);
    nextRefresh=((time()+FREEZE_DELAY/2)/REFRESH)*REFRESH; //Freeze intermediate half time
  }

   void updateData(){
    //Check if timing mode has changed
    if (timing.mode & 0xF0 != ntiming.mode & 0xF0) {
      //Check if we are in parallel mode 
      if (timing.mode & MII_MODE_PARALLEL) {
        //Set SDA and SCL to input and catch the interrupt
        //pinMode(MII_PIN_SDA,INPUT);
        //pinMode(MII_PIN_SCL,INPUT);
        
      }
    }
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
                  } else Printer.tab(23);
                  Printer.time(timing.times[i].time);
                }
                Printer.println();
              } else { //State >=DNF
                Printer.print(F("Manual"));
                Printer.tab(17);
                Printer.print(timing.times[i].user);
                Printer.tab(23);
                Printer.println(timing.times[i].state==MII_STATE_DNF ? F("DNF") :
                   timing.times[i].state<=MII_STATE_DSQ_R ? F("DSQ") :
                   timing.times[i].state<=MII_STATE_DNS_R ? F("DNS") : F("INV"));           
              }
            }
         }
        }
      }
    }
   #endif  
    MiiRFClient::updateData();
  }

  #if MII_PRINTER
  switchRec_t switches[MII_MAX_GATES];
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
    MiiRFClient::afterSend();
  }
 #endif
  //Catch the swith command
  bool beforeProcess(){ 
   uint8_t len; 
   #if MII_ESP
     len=MII_SIZEOF_timing_t;
     recv((uint8_t*) &smallbuf[0],&len,true);
     Esp.send((uint8_t)_rxHeaderId,(uint8_t*)&smallbuf[0],len);
   #endif
   #if MII_PRINTER   
    if (_rxHeaderId==MII_CMD_SWITCH) {    
        uint32_t time;
        len=sizeof(uint32_t);
        if (recv((uint8_t*) &time,&len,true)) {
          logSwitch(_rxHeaderFrom,time);  
        }
    }
   #endif  
   return MiiRFClient::beforeProcess();
  }

  void printTime(uint8_t x, uint8_t y,uint32_t t,bool showMills) {
     t=(t+MII_ROUNDING)/10; //Rounding and remove thousands
     uint8_t w = lcd.getFontWidth()+1;     
     uint16_t h = (t/360000)%60;
     if (h>0) {
        lcd.setCursor(x,y);
        lcd.print(h%24);
        //now draw 2 dot's
        x=lcd.getX();
        lcd.drawPixel(x,y+3,1);
        lcd.drawPixel(x,y+4,1);
        lcd.drawPixel(x,y+6,1);
        lcd.drawPixel(x,y+7,1);
        x+=2;
     } else { //Skip
        x+= w+2;
     }
     lcd.setCursor(x,y);
       
     uint16_t m = (t/6000)%60;
     //Minute
     if (m>0 || h>0) {
        if (m<10 && h>0) lcd.print('0');
        if (m<10) x+=w;
        lcd.setCursor(x,y);
        lcd.print(m);
        //now draw 2 dot's
        x=lcd.getX();
        lcd.drawPixel(x,y+2,1);
        lcd.drawPixel(x,y+3,1);
        lcd.drawPixel(x,y+6,1);
        lcd.drawPixel(x,y+7,1);
        x+=2;
     } else { //Skip
        x+=(w*2) + 2;
     }
    
     //Second
     lcd.setCursor(x,y);
     uint16_t s= (t/100)%60;
     if (s<10 && (h>0 || m>0)) {
       lcd.print('0');
     } 
     if (s<10) x+=w;
     lcd.setCursor(x,y);
     lcd.print(s);
     x=lcd.getX(); 
     lcd.drawPixel(x,y+lcd.getFontHeight()-1,1);
     x+=2;
     if (showMills){
       lcd.setCursor(x,y);
       t=t%100;
       if (t<10) lcd.print('0');
       lcd.print(t);
     }   
  }
  
  void printState(uint8_t x, uint8_t y,uint8_t state) {
    lcd.setCursor(x,y);
    switch (state) {
    //   case 0             : lcd.print(F("NEXT"));break;
       case MII_STATE_DNF : lcd.print(F("DNF"));break;
       case MII_STATE_DNS : lcd.print(F("DNS"));break;
       case MII_STATE_DSQ : lcd.print(F("DSQ"));break; 
       case MII_STATE_FINISH_PREV: lcd.print(F("L+"));break;
       case MII_STATE_FINISH_LAST: lcd.print(F("R+"));break;
       case MII_STATE_REMOVED : lcd.print(F("DEL"));break;   
     }
  }
  
  void printUser(uint8_t x, uint8_t y,uint16_t id){
    lcd.setCursor(x,y);
    id%=1000;
    if (id<100) lcd.print(' ');
    if (id<10) lcd.print(' ');
    lcd.print(id);
  }
  
  int printSmallBufTime(int pos) {
    lcd.print(smallbuf[pos++]);lcd.print(smallbuf[pos++]);lcd.print(':'); 
    lcd.print(smallbuf[pos++]);lcd.print(smallbuf[pos++]);lcd.print(':'); 
    lcd.print(smallbuf[pos++]);lcd.print(smallbuf[pos++]);
    return pos;
  }
  
  void printActive() {
     //Display the next user and active count
      lcd.setFont(minimal_5x5);
      printUser(38,0,nextUser);
      lcd.drawFastVLine(54,1,3,1);
      lcd.setCursor(56,0); //User is 3 long
      lcd.print(activeCount);
  }

  
    //Special function for showing time
  void draw(timeRec_t &rec,boolean showMills){
    lastRec = rec;
    lastMills= showMills;
    nextRefresh=((time()+REFRESH)/REFRESH)*REFRESH; //Update timer every  .5 seconds
    lcd.clear();
    
    uint32_t t = ((rec.state>=MII_STATE_FINISH && rec.state<MII_STATE_DNF)  || showMills
       ? rec.time
       : time() - rec.start);
  
    lcd.setFont(minimal_5x5);   
     //On Parallel mode we show P before user id
    if (isParallelMode()) {
       lcd.print(F("P")); 
    } 
   
   if (rec.state<0xff) {  //Display the start nr and time
      printUser(isParallelMode() ? 6 : 0,0,rec.user);
          
      
      if (rec.state>=MII_STATE_DNF) {
            lcd.setFont(small_5x7);
            lcd.setTextSize(2);
            printState(24,1,rec.state);
      } else {
        
        if (showMills && rec.state<MII_STATE_FINISH) {
          lcd.setCursor(lcd.getX()+lcd.getFontWidth(),0);
          lcd.print('I');
        }   
        

        //Display the run time 
        lcd.setFont(digitals_11x7);
        printTime(3,5,t,(rec.state>=MII_STATE_FINISH && rec.state<MII_STATE_DNF)|| showMills); 
//        printState(lcd.getX()+lcd.getFontWidth(),0,rec.state);    //State would overlap

        //Show Lane on parallel mode
        if (isParallelMode() && (rec.state>=MII_STATE_FINISH && rec.state<MII_STATE_DNF)) {
           lcd.setFont(small_5x7);
           printState(0,9,rec.state);
        }
      
      //Display the next user and active count
      printActive();
     } 
      
    } else { //Show time marker olny
      lcd.setCursor(46,0);
      lcd.setFont(minimal_5x5);
      lcd.print(F("T"));
      lcd.setCursor(49,0);
      lcd.print(F("IME"));
  
      if (time()==0) {
        lcd.setFont(small_5x7);
        lcd.setCursor(3,5);
        lcd.print(F("SYNC"));
      } else {
        lcd.setFont(digitals_11x7);
        printTime(3,5,time()+(timing.sessionDate%86400)*1000,false); 
      }
    }

    lcd.drawPixel(WIDTH-1,0,activityDot ? 0 : 1); 
    lcd.display();
 }

    //Called by main loop to do the internal processing
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
    uint32_t _time = time();
    if (!isReady() || _time==0 ) return;
    //Check for the count down clock
    if (countdownVal==0 && _countdown && _countdown+3000<_time) { //After 5 Seconds we lock gate again
     #ifdef MII_PIN_GATE
      digitalWrite(MII_PIN_GATE,HIGH); 
      #endif
      _countdown=0;
    }
    if (_countdown+1000>_time && (activeTime.start==0 || _thisAddress==MII_DEV_SCOREBOARD_COUNT_DOWN) ) { //...3, 2, 1, GO
       uint8_t cVal = (_countdown+999-_time)/1000;
       if (cVal!=countdownVal) {
         lcd.clear();
         lcd.setCursor(22,1);
         lcd.setFont(small_5x7);
         lcd.setTextSize(2);
         if (countdownVal==0) {
          lcd.setCursor(4,1);
          lcd.print(F("READY"));
         } 
         if (cVal>0) {
           lcd.print(cVal);
         } else {
           lcd.print(F("GO"));
            #ifdef MII_PIN_GATE
           //We unlock the gates by pulling 
            digitalWrite(MII_PIN_GATE,LOW); 
           #endif 
         }
         lcd.display();
         countdownVal=cVal;
       }
    } else if (_time>nextRefresh && activeTime.start!=0) {
        draw(activeTime,false);
        nextRefresh=((_time+REFRESH)/REFRESH)*REFRESH; 
    }else if ((_time>nextRefresh) && activeTime.start==0
               && (finishTime[0].start==0 ||  _time>finishTime[0].start+finishTime[0].time+RELEASE_TIME)) 
    { //Empty start list but connected show clock
      lastRec.state=0xff;
      lastRec.user=0;
      lastRec.state=0xff;
       #if MII_CLOCK
      lastRec.time=timing.sessionDate==0 ? (now()%86400)*1000  : _time+(timing.sessionDate%86400)*1000; 
      #else 
      lastRec.time=_time+(timing.sessionDate%86400)*1000; //Sessiondate is converted to 24Hours and then to millis
      #endif
      draw(lastRec,false);
      nextRefresh=((_time+REFRESH)/REFRESH)*REFRESH; //Update timer every  .5 seconds
    } else if (lastRec.state<MII_STATE_DNF && (lastActiveCount!=activeCount || lastNextUser!=nextUser)) {
       printActive(); //Redraw because of count is changing
       lcd.display();
    }
    
    lastActiveCount=activeCount;
    lastNextUser=nextUser;
    #ifdef MII_PIN_KEYBOARD
      //Read keyboard
      uint8_t key = getKey();
      if (key==1 && activeCount!=0) setDNF();
      else if (key==2) noFinish();
      else if (key==3) setMode(isTimeMode() ? MII_MODE_PARALLEL : MII_MODE_TIME);  //Switch the time mode
    #endif
  }
   
  uint8_t finishPos;
  boolean activityDot;  
  timeRec_t lastRec; 
  boolean lastMills; 
  uint16_t lastNextUser;
  uint8_t lastActiveCount;
  uint8_t  countdownVal;
};

//Init the classes we will use
MiiRFHost rf(MII_PIN_RF_CS,MII_PIN_RF_IRQ,MII_PIN_RF_SDN);

void yield(){
  lcd.scan();
}  

void setup() {
  
  // put your setup code here, to run once:
  lcd.begin(small_5x7,displaybuf, WIDTH, HEIGHT);
  lcd.setDrawbuf(drawbuf);
  lcd.display();
  
  #if MII_CLOCK
   // RTC.set(1431961440L);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
  #endif
    
  //Read the config  
  readConfig(MII_DEVICE_ID,MII_DEV_SCOREBOARD - MII_DEV,MII_TXPOW_17DBM,MII_FIRMWARE);
  
  //As of version 1.2 we have screws, time and keyboard
  /*
  #if MII_VERSION >= 1012
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
        MiiConfig.address=MII_DEV_SCOREBOARD-pos;
     }
  #endif 
  */
  //Show the config settings
   lcd.clear();
 //  lcd.setTextSize(1);
   lcd.print(MiiConfig.group);lcd.print('.');lcd.print(MiiConfig.channel);lcd.print('.');lcd.println(MiiConfig.address);
   lcd.print(F("Connecting"));  
   lcd.display();
   
   if (!rf.init(MiiConfig.address,MiiConfig.group,MiiConfig.channel,MiiConfig.power,MiiConfig.address==MII_DEV_MASTER)) {
      lcd.clear();
      lcd.print(F("Failure"));  
      lcd.display();
      delay(5000);
     RESTART();
   }

  #if MII_ESP
    Esp.begin();
  #endif  
   

  
  #if MII_PRINTER
    //pinMode(MII_PIN_BTX,INPUT);
    //sSerial.begin(9600);
    sSerial.begin(MII_PRINTER_BAUD);
    Printer.begin("MSP-100",1234);
  #endif

  //Pull up the GATE pin if defined
  #ifdef MII_PIN_GATE
   pinMode(MII_PIN_GATE,OUTPUT);
   digitalWrite(MII_PIN_GATE,HIGH);
  #endif

  //Read the analog base for keyboard
  #ifdef MII_PIN_KEYBOARD
    pinMode(MII_PIN_KEYBOARD,INPUT);
    keyBase=analogRead(MII_PIN_KEYBOARD);
  #endif
}

void loop() {
  
  // put your main code here, to run repeatedly:
  if (rf.available()) {
   rf.recv(); //Make sure we remove the message
  }
  rf.doProcess();

  //Do the printer processing
  #if MII_PRINTER
    Printer.doProcess();
  #endif
 yield();


}
