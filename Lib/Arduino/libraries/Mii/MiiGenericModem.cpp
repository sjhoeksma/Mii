#include <MiiGenericModem.h>
#include <SPI.h>

#if defined MPIDE
#include <peripheral/int.h>
#define memcpy_P memcpy
#define ATOMIC_BLOCK_START unsigned int __status = INTDisableInterrupts(); {
#define ATOMIC_BLOCK_END } INTRestoreInterrupts(__status);
#elif defined ARDUINO
#include <util/atomic.h>
#define ATOMIC_BLOCK_START     ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#define ATOMIC_BLOCK_END }
#endif


#if MII_WATCHDOG
#include <avr/wdt.h> //Watch dog
/*
// watchdog ISR
ISR (WDT_vect)
{
  wdt_disable();  // disable watchdog
  asm volatile ("  jmp 0"); //Restart
}
// end of WDT_vec
*/
#endif

// Interrupt vector
MiiGenericModem* MiiGenericModem::_self = 0;

MiiGenericModem::MiiGenericModem(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
    :
    _mode(MII_RF_MODE_IDLE), // We start up in idle mode
    _selectPin(selectPin),
    _intPin(intPin),
    _sdnPin(sdnPin),
    _timeSync(true),
    _thisAddress(MII_BROADCAST_ADDRESS),
    _txHeaderTo(MII_BROADCAST_ADDRESS),
    _txHeaderFrom(MII_BROADCAST_ADDRESS),
    _txHeaderId(0),
    _txHeaderFlags(0),
    _rxBufValid(0),
    _drift(MII_RF_DEFAULT_DRIFT),
    _airTime(MII_RF_MIN_MSG_INTERVAL),
    _timeout(MII_RF_DEF_TIMEOUT),
    _timeInterval(MII_TIME_INTERVAL_CLIENT),
    _retryCount(MII_RF_DEFAULT_RETRY){
  #if MII_WATCHDOG
  wdt_disable();
  #endif
    _self=this;
}

void MiiGenericModem::clearData(){
#if MII_MAX_ADDRESS
  _addressCount=0;
#endif
}

// These are low level functions that call the interrupt handler for the correct
void MiiGenericModem::isr() {
    _self->handleInterrupt();
}

bool MiiGenericModem::init(){
    //Default init procedure
    _ready=false;
    powerOn();

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
#if F_CPU <= 8000000
    SPI.setClockDivider(SPI_CLOCK_DIV8);
#else
    SPI.setClockDivider(SPI_CLOCK_DIV16);
#endif
    SPI.begin();
     #if SPI_ISOLATE
    _SPCR = SPCR;
    _SPSR = SPSR;
    #endif

    // Initialise the slave select pin
    pinMode(_selectPin, OUTPUT);
    digitalWrite(_selectPin, HIGH);
    delay(100); //Give some time
    // On all other platforms, its innocuous, belt and braces
    pinMode(_intPin, INPUT);

  #if MII_WATCHDOG
   /*
   MCUSR &= ~(1<<WDRF);    // Clear the reset flag.
   WDTCSR |= (1<<WDCE) | (1<<WDE);  //In order to change WDE or the prescaler, we need to set WDCE (This will allow updates for 4 clock cycles).
   WDTCSR = 1<<WDP0 | 1<<WDP3; // set new watchdog timeout prescaler value 8.0 seconds
   WDTCSR |= _BV(WDIE);  // Enable the WD interrupt (note no reset).
   */
   wdt_enable(WDTO_8S); //Watch dog 8 seconds
  #endif
    return true;
}

bool MiiGenericModem::recv(uint8_t* buf, uint8_t* len,bool peek){
  bool ret = false;
  #if MII_MAX_MSG_CACHE
   if (_msgCount) ret=popMsg(buf,len,peek);
  #endif
  if (!ret) { //We had no collision to process, see if there is other data
    ret = internalRecv(buf,len,peek);
  }
  if (ret && !peek) {
     if (_rxHeaderFrom!=MII_BROADCAST_ADDRESS && _rxHeaderTo==_thisAddress &&
         (_rxHeaderFlags & MII_RF_FLAGS_ACK) && _rxTime!=_ackTime) {
       #if MIIRF_SERIAL
        Serial.print("Sending Ack:");Serial.print(_rxHeaderId);Serial.print(' ');Serial.println(_rxHeaderFrom);Serial.flush();
       #endif
       _ackTime=_rxTime;
       cbi(_rxHeaderFlags,MII_RF_FLAGS_ACK); //Clear Ack Flag
       setHeaderId(MII_C_ACK);
       setHeaderFlags(0);
       setHeaderTo(_rxHeaderFrom);
       send((uint8_t*)&_rxHeaderId, sizeof(_rxHeaderId));
       waitPacketSent();
    }
  }
  return ret;
}

// Blocks until a valid message is received or timeout expires
// Return true if there is a message available
// Works correctly even on millis() rollover
bool MiiGenericModem::waitAvailable(uint16_t timeout){
    uint32_t exittime = millis()+timeout;
    while (timeout==0 || exittime >= millis()) {
        if (available())
           return true;
        YIELD;
    }
    return false;
}

bool MiiGenericModem::waitPacketSent(uint16_t timeout){
    uint32_t exittime = millis()+timeout;
    while (timeout==0 || exittime >= millis()) {
        if (_mode != MII_RF_MODE_TX) // Any previous transmit finished?
           return true;
       YIELD; // Wait for any previous transmit to finish
    }
    setModeRx(); //Error return to transmit mode
    return false;
}

void MiiGenericModem::setPromiscuous(bool promiscuous){
    _promiscuous = promiscuous;
}

void MiiGenericModem::setAddress(uint8_t address){
  _thisAddress=address;
  setHeaderFrom(address);
  #if MII_MAX_ADDRESS
   setPromiscuous(true); //We will filter the address ourself in available
  #endif
  if (!_timeInterval)
    _timeInterval=_timeSync ? MII_TIME_INTERVAL_CLIENT : MII_TIME_INTERVAL_NOTIMESYNC;
}

void MiiGenericModem::setChannel(uint8_t channel){
 //Convert kHz to MHz
  float freq= MII_RF_CHANNEL_BASE  + (channel % MII_RF_CHANNEL_COUNT)*(MII_RF_CHANNEL_RANGE / MII_RF_CHANNEL_COUNT);
  setFrequency(freq, MII_RF_CHANNEL_WIDTH);
}

void MiiGenericModem::setMaster(uint8_t address){
  _masterAddress=address;
  if (address==_thisAddress) {
    setTimeSync(true);
    _timeInterval= MII_TIME_INTERVAL_MASTER;
  }
}

void MiiGenericModem::setHeaderTo(uint8_t to){
    _txHeaderTo = to;
}

void MiiGenericModem::setHeaderFrom(uint8_t from){
    _txHeaderFrom = from;
}

void MiiGenericModem::setHeaderId(uint8_t id){
    _txHeaderId = id;
}

void MiiGenericModem::setHeaderFlags(uint8_t set, uint8_t clear){
    _txHeaderFlags &= ~clear;
    _txHeaderFlags |= set;
}

uint8_t MiiGenericModem::headerTo(){
    return _rxHeaderTo;
}

uint8_t MiiGenericModem::headerFrom(){
    return _rxHeaderFrom;
}

uint8_t MiiGenericModem::headerId(){
    return _rxHeaderId;
}

uint8_t MiiGenericModem::headerFlags(){
    return _rxHeaderFlags;
}

int8_t MiiGenericModem::lastRssi(){
    return _lastRssi;
}

uint8_t  MiiGenericModem::mode(){
    return _mode;
}

uint32_t  MiiGenericModem::rxTime(){
    return _rxTime;
}

uint32_t  MiiGenericModem::txTime(){
    return _txTime;
}

void MiiGenericModem::setTimeSync(bool state){
  _timeSync=state;
}

bool MiiGenericModem::getTimeSync(){
  return _timeSync;
}

void MiiGenericModem::setRetries(uint8_t retries){_retryCount=retries;} //Set default retry sendAckCmd
void MiiGenericModem::setTimeout(uint16_t timeout){_timeout=timeout;} //Set default timeout for WaitForCmd
void MiiGenericModem::setDrift(uint16_t drift){_drift=drift;}   //Set the time allowed to run appart in ms, before a full time sync will be done again
void MiiGenericModem::setTimeInterval(uint32_t interval){ _timeInterval = interval;} //Set the time update interval

//Just wait idle
void MiiGenericModem::idle(long timeout) {
  YIELD;
  if (timeout<=0) return;
  uint32_t t=timeout+millis();
  while (t>millis()){
   YIELD;
  }
}

uint8_t MiiGenericModem::spiRead(uint8_t reg){
    uint8_t val;

    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg & ~MII_RF_SPI_WRITE_MASK); // Send the address with the write mask off
    val = SPI.transfer(0); // The written value is ignored, reg value is read
    chipSelectHigh();
    ATOMIC_BLOCK_END;
    return val;
}

void MiiGenericModem::spiWrite(uint8_t reg, uint8_t val){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg | MII_RF_SPI_WRITE_MASK); // Send the address with the write mask on
    SPI.transfer(val); // New value follows
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}

void MiiGenericModem::spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg & ~MII_RF_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
      	*dest++ = SPI.transfer(0);
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}

void MiiGenericModem::spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg | MII_RF_SPI_WRITE_MASK); // Send the start address with the write mask on
    while (len--)
      	SPI.transfer(*src++);
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}


void MiiGenericModem::chipSelectHigh(void){
  digitalWrite(_selectPin, HIGH);
  SPDR = 0xFF;
  while (!(SPSR & (1 << SPIF)));
  #if SPI_ISOLATE
   SPCR=_SPCRO;
   SPSR=_SPSRO;
  #endif
}

void MiiGenericModem::chipSelectLow(void){
 #if SPI_ISOLATE
  _SPCRO=SPCR;
  _SPSRO=SPSR;
  SPCR = _SPCR;
  SPSR = _SPSR;
 #endif
 digitalWrite(_selectPin, LOW);
}

bool MiiGenericModem::isReady(void){
 return _ready;
}

 //Discard ad message by receving the message without ack
bool MiiGenericModem::discard(){
 bool ret = false;
   #if MIIRF_SERIAL >= 1
     Serial.println("Discard()");Serial.flush();
   #endif

  #if MII_MAX_MSG_CACHE
   ret=popMsg();
   pushMsg(); //load next headers
  #else
  if (!ret) { //We had no collision to process, see if there was normal data
    if (!hasAvailable()) return false; //We don't want any processing
    clearRxBuf();
    _rxHeaderTo=0;
    ret = true;
  }
  #endif
  return ret;
}

//Send a command and wait for Ack when trys is set
bool  MiiGenericModem::sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t trys,uint8_t flags){
  bool ret=false;
  if (trys==0xFF) trys = _retryCount; //trys will be on more than number of retries
  if (trys) trys++; //We always loop so 1 try is always needed
  uint8_t _trys=trys/2;
  if (!address) address=_masterAddress; //When no address is specified we user master
  if (!address) return false; //We will not send data to a undefined address
  do  {
	  setHeaderFlags(trys && (address!=MII_BROADCAST_ADDRESS) ? (MII_RF_FLAGS_ACK | flags) : flags);
    setHeaderId(cmd);
    setHeaderTo(address);
    //When len is 0 we only send cmd as data
    ret = len> 0 ? send(buf, len) : send(&cmd,sizeof(cmd));
    #if MIIRF_SERIAL >= 2
      Serial.print("Send len: ");Serial.println(len);Serial.flush();
    #endif
    waitPacketSent();
    if (ret && trys && address!=MII_BROADCAST_ADDRESS) {
     //Wait for ACK
     ret = waitForCmd(MII_C_ACK,0,0,address);
     //reduce the retry counts
     trys--;
     #if MIIRF_SERIAL
       if (!ret && trys>0) Serial.println(F("RETRY"));
     #endif

     //For using Relaying address is needed for forwarding cache is needed
     #if MII_MAX_ADDRESS
     //We relay during try's
     if (!ret && (flags & MII_RF_FLAGS_RELAY) && trys<_trys){
        uint8_t ca;
        if ((ca=closestAddress(_thisAddress))) address=ca;
     }
     #endif

    }
  } while (!ret && trys>0);

  //Return to listening
  setModeRx();
  return ret;
}

//Send a command to a specific address
bool MiiGenericModem::sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t flags){
 return sendAckCmd(cmd,buf,len,address,0,flags);
}

bool MiiGenericModem::sendCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,0,flags);
}

bool MiiGenericModem::sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,trys,flags);
}

bool MiiGenericModem::waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  if (!address) address=_masterAddress;  //We use master address for ok when not set
  uint32_t failtime=millis()+timeout;
  //We wil not acknowledge messages
  while (failtime>millis()){
      if (available()) {
       if (_rxHeaderId==cmd && (address==MII_BROADCAST_ADDRESS || address==_rxHeaderFrom)) {
          return recv(buf,&len); //We are allowed to send ack we have received it
       }
       discard();
    } else
      YIELD;
  }
  return false;
}


#if MII_MAX_MSG_CACHE
bool MiiGenericModem::pushMsg(){
 bool ret = false;
 if (hasAvailable() && _msgCount<MII_MAX_MSG_CACHE) {
    _msg[_msgCount].headerId=_rxHeaderId;
    _msg[_msgCount].headerFrom=_rxHeaderFrom;
    _msg[_msgCount].headerTo=_rxHeaderTo;
    _msg[_msgCount].headerFlags=_rxHeaderFlags;
    _msg[_msgCount].length=MII_RF_MESSAGE_LEN;
    _msg[_msgCount].rxTime=_rxTime;
    if (internalRecv(_msg[_msgCount].data,&_msg[_msgCount].length)) {//We will allow ack to be send
       _msgCount++; //We have read a new collision
      ret = true;
    }
    #if MIIRF_SERIAL
     Serial.print("pushMsg():");Serial.println(_msgCount);Serial.flush();
    #endif
  }
  if (_msgCount) {
    //Fill the headers with the data available
    _rxHeaderId=_msg[0].headerId;
    _rxHeaderFrom=_msg[0].headerFrom;
    _rxHeaderTo=_msg[0].headerTo;
    _rxHeaderFlags=_msg[0].headerFlags;
    _rxTime=_msg[0].rxTime;
  }
  return ret;
}

bool MiiGenericModem::popMsg(uint8_t* buf, uint8_t* len,bool peek){
  if (_msgCount) {
    #if MIIRF_SERIAL
     Serial.print("popMsg():");Serial.println(_msgCount);Serial.flush();
    #endif
     if (buf && len){
       *len=min(*len,_msg[0].length);
       memmove(buf,_msg[0].data,*len);
      };
     if (peek) return true;

     //Check if we are in receiving mode and have already data in.
     //If so we have to move it in and out
      uint8_t rxHeaderId=_msg[0].headerId;
      uint8_t rxHeaderFrom=_msg[0].headerFrom;
      uint8_t rxHeaderTo=_msg[0].headerTo;
      uint8_t rxHeaderFlags=_msg[0].headerFlags;
      uint32_t rxTime=_msg[0].rxTime;

      ATOMIC_BLOCK_START;
      _msgCount--;
      memmove(&_msg[0],&_msg[1],sizeof(msgRec_t)*_msgCount);
      ATOMIC_BLOCK_END;
      //We have not room again in Collision stack see if we should push received record
      pushMsg();

      //We save record in collision so now we can swap headers
      _rxHeaderId=rxHeaderId;
      _rxHeaderFrom=rxHeaderFrom;
      _rxHeaderTo=rxHeaderTo;
      _rxHeaderFlags=rxHeaderFlags;
      _rxTime=rxTime;
      return true;
  }
  return false;
}
#endif

bool MiiGenericModem::syncTime(uint8_t address){
   if (isMaster()) return false;
//   _timeDiff=0; //Block sending data
   //Send a request for timesync to the MASTER, on receving the request Master will only accept messages from this device
   //Master will send his Time T[0] to this device will receive at T[1] and we will response with new request, skipped because of start master
   //Master will send his Time T[2] to this device will receive at T[3] and we will response with new request
   //Master will send his Time T[4] to this device will receive at T[5] and start calculation
   uint32_t T[6];
   uint8_t toAddress=MII_BROADCAST_ADDRESS;
   for (uint8_t i=0;i<6;){
     if (!(sendCmd(MII_C_TIMESYNC,(uint8_t*)&address,sizeof(uint8_t)) &&
           waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),address))
     ) {
      _lastTime+=1000; //Failed try it again in one second
      return false;
    }
     i++; //The send time
     if (address==MII_BROADCAST_ADDRESS) address=_rxHeaderFrom; //toAddress can be broadcast for next rounds we take the real address
     T[i++]=_rxTime; //The receive time
   } //For
   _masterAddress=address; //Valid time so set the masterAddress
   //Calculate transmit time by dividing round trip using the two times (T[2] and T[4]) of master by 2
   //Add this round trip to the T[4]send time of master and substract the receive time T[5]
   _airTime=(T[4]-T[2])/2;
   _timeDiff=T[4]+_airTime-T[5];
   _timeout=(_airTime*2)+MII_RF_MIN_MSG_INTERVAL*3;
   //We will resend time quicker to all because we have synced
   _lastTime=millis(); //Good sync
   timeChanged();
   #if MIIRF_SERIAL >= 2
      Serial.print("Sync Time:");Serial.print(millis());Serial.print('-');Serial.print(_timeDiff);Serial.print('-');Serial.println(_airTime);Serial.flush();
    #endif
   return true;
}


#if MII_C_CHANNEL
void MiiGenericModem::changeChannel(uint8_t channel){ //Change the channel only master by sending command to all clients
if (isMaster()) {
    //Master will broadcast SET_CHANNEL for 10 times, 1 second intervall to all listening
    for (int i=0;i<10;i++) {
      sendCmd(MII_C_CHANNEL,&channel,sizeof(uint8_t));
      delay(1000);
    }
  }
  setChannel(channel);
}
#endif

bool MiiGenericModem::internalProcess(){
 bool ret = false;
 if (_inAvailable || available()) { //Prevent recusion
   #if MIIRF_SERIAL >= 1
     Serial.print("New:");Serial.print(_rxHeaderFrom);Serial.print(' ');Serial.println(_rxHeaderId);Serial.flush();
   #endif

   #if MII_MAX_ADDRESS
     addAddress(_rxHeaderFrom,_lastRssi);
	 #endif

	 //Filter the messages, to see if they are for us
	 if (!beforeProcess() || _rxHeaderFrom == _thisAddress ||
	    !(_rxHeaderTo == _thisAddress || _rxHeaderTo == MII_BROADCAST_ADDRESS)
	    ) { //Discard the message, it is not for us, no ack
	    discard();
	    return false;
	 }


   //Relay only works with caching enabled
   #if MII_MAX_MSG_CACHE
	 //Check if we should relay message
	 if ( (_rxHeaderFlags & MII_RF_FLAGS_RELAY) && !isMaster()) {
	    if (_masterAddress) { //We should relay the message to master
	     //Relay the command, by reusing the active message
	      sendAckCmd((uint8_t)_rxHeaderId,(uint8_t*)&_msg[0].data,_msg[0].length,_masterAddress,(uint8_t)0xFF,(uint8_t)(_rxHeaderFlags & ~_BV(MII_RF_FLAGS_RELAY)));
	    }
	    //Drop the messages
	    discard();
	 } else
	 #endif

	 switch (_rxHeaderId){
    case MII_C_TIMESYNC :
       recv(); //Clear TimeSync CMD
       if ( isMaster() || _rxHeaderTo==_thisAddress) { //Only master or direct requests will reply on Time Sync commands
         //Master will send 3 times data, no retry and will not wait for last reqst
         uint32_t t;
         for (uint8_t i=0;i<3;i++) {
           t=time();
           if (!sendCmd(MII_C_TIMESYNC,(uint8_t*)&t,sizeof(uint32_t),_rxHeaderFrom)) break;
           if (i==2 || !waitForCmd(MII_C_TIMESYNC,0,0,_rxHeaderFrom)) break; //Exit if did not send or got correct response
         }
         //As Master we will send a time command close .7Sec after we did do a sync
         if (isMaster() && t>_timeInterval) _lastTime=t+700-_timeInterval;
       } else {
          //When somebody is doing time sync make sure we don't send a message
          _txTime=millis()+_airTime*2;
       }
       break;

   case MII_C_REQ_TIME:
      recv(); //Clear Request Time CMD
      if (isMaster()) sendTime();
      break;

   //Internal command to set transmit power
   case MII_C_POWER:{
      uint8_t len=sizeof(uint8_t);
      uint8_t power;
      if (recv(&power,&len)) setTxPower(power);
      break;
   }


   #if MII_C_CHANNEL
   case MII_C_CHANNEL: {
         uint8_t channel;
         if (recv(&channel,sizeof(uint8_t)) changeChannel(channel);
    } break;
    #endif

    case MII_C_TIME:
        if (!isMaster()) { //We are just a client and should process this time from master
           uint32_t _time;
           uint8_t _len=sizeof(_time);
           if (recv((uint8_t *)&_time,&_len) && _time) { //Only accept time if it is set
             long diff = time(_rxTime)-_airTime-_time;
             #if MIIRF_SERIAL >= 2
              Serial.print("Time:");Serial.print(time(_rxTime));Serial.print("-");Serial.print(time(_airTime));Serial.print("-");Serial.print(_time);Serial.print("=");Serial.print(diff);Serial.print('<');Serial.print(_drift);Serial.print(" [");Serial.print(millis());Serial.print('-');Serial.print(_rxTime);Serial.print('-');Serial.print(_airTime);Serial.println("]");Serial.flush();
             #endif
              if (!getTimeSync()) {
               _masterAddress=_rxHeaderFrom; //Valid time so set the masterAddress
               _timeDiff-=diff;
               _lastTime=_rxTime;
               if (abs(diff)>_drift) timeChanged();
              //We balance the time drift by adding half of the drift if difference is within 10ms
              } else if (abs(diff)<=_drift && _timeDiff) { //We need to valid timediff before we calculate diff
                 _timeDiff-=diff; //((diff*3)/4); //For bigger drifts gap is closed better (.75)
                 _lastTime=_rxTime;
              } else { //When difference to large request timing
                syncTime();
              }
           }
        } else {
          discard(); //Clear Time CMD, without ack
        }
        break;
    default : ret=true; break; //We have not internal processed so it is available
    }
  }
  return ret;
}


bool MiiGenericModem::hasAvailable(){
 if (!_rxBufValid) {
   if (_mode==MII_RF_MODE_TX) return false;
	 setModeRx(); // Make sure we are receiving
 }
 return _rxBufValid;
}

bool MiiGenericModem::available(){

  #if MII_WATCHDOG
   wdt_reset(); //Tell watch dog everything is ok
  #endif

  //If we allow collision the be retrieved, check if we have them available
  bool ret = false;
  #if MII_MAX_MSG_CACHE
   if (_msgCount) {
     pushMsg(); //If there is real data just pusch it on the stack and restore next headers
     ret=true;
   } else {
    ret= hasAvailable(); //Make sure we are in the correct mode
   }
  #else
   ret = hasAvailable();
  #endif
  if (_inAvailable) return ret;
  if (ret && !_inAvailable) {
     _inAvailable=true;
     ret = internalProcess();
     //Unlock the available loop
     _inAvailable=false;
  }
  if (!ret) { //Now data was available to process
    //  check if we should do time sync, when never registered do it fast
    if (_timeInterval && !isMaster() && (_lastTime+(_timeDiff==0 ? _timeInterval/3 : _timeInterval))<millis()){
     #if MIIRF_SERIAL >= 2
         Serial.print("Request NEW TIME:");Serial.print(_timeInterval);Serial.print('+');Serial.print(_lastTime);Serial.print('<');Serial.println(millis());
      #endif
     _lastTime=millis();
     if (!getTimeSync()) {
        if (_timeDiff==0) sendCmd(MII_C_REQ_TIME);
      } else {
       syncTime();
      }
    }
    //Check for master and if we should send update of time, not during sync
    if (_timeInterval && isMaster() && millis()>_lastTime+_timeInterval){
      // Serial.println("Sending master time");
      sendTime();
    }
    YIELD;
  }
  return ret;
}

void MiiGenericModem::sendTime() {
  if (isMaster()) {
    setHeaderFlags(0);
    setHeaderId(MII_C_TIME);
    setHeaderTo(MII_BROADCAST_ADDRESS);
    uint32_t _time = time();
    if (send((uint8_t*)&_time,sizeof(_time))) {
       _lastTime=millis();
       waitPacketSent();
    }
  }
}

uint32_t MiiGenericModem::time(uint32_t _time){
 return isMaster() ? (_time ? _time : millis()) : (_timeDiff ? (_time ? _time : millis())+ _timeDiff : _time );
}

bool MiiGenericModem::isMaster(){
  return _masterAddress==_thisAddress;
}


#if MII_MAX_ADDRESS
uint8_t MiiGenericModem::nextAddress(uint8_t address){
  if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list and our own address
  uint8_t ret=_thisAddress>address ? _thisAddress :0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id>address)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiGenericModem::prevAddress(uint8_t address){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address)
      ret=(ret==0 ? _address[i].id : max(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiGenericModem::firstAddress(uint8_t address){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}

uint8_t  MiiGenericModem::closestAddress(uint8_t address) {
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=0;
  uint8_t rssi=0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].rssi>rssi && _address[i].id<MII_DEV_CLIENT) {
      rssi= _address[i].rssi;
      ret = _address[i].id;
    }
  }
  return ret;

}

void MiiGenericModem::firstLastAddress(uint8_t &firstClient,uint8_t &lastClient,uint8_t &clientCount){
    lastClient = min(_thisAddress,MII_DEV_CLIENT);
    firstClient =lastClient;
    clientCount = 0;
    for (uint8_t i=0;i<_addressCount;i++) {
      if (_address[i].id<MII_DEV_CLIENT) {
        clientCount++;
        if (_address[i].id>lastClient) lastClient = _address[i].id;
        if (_address[i].id<firstClient) firstClient = _address[i].id;
      }
    }
}

void MiiGenericModem::addAddress(uint8_t address,uint8_t rssi){
   //We have data build up the addressTable
   uint8_t a=0;
   for (a=0;a<_addressCount && _address[a].id!=address;a++){}
   if (a<_addressCount) { //We found the record, shift first records to freeup record
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*a);
   } else { //New Record, insert at first place
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*_addressCount);
     if (_addressCount<MII_MAX_ADDRESS-1) _addressCount++;
   }
   //Fill the first record with data for recieved address
   _address[0].id=address;
   _address[0].seen=millis(); //Create seconds
   _address[0].rssi=rssi;
}

#endif
