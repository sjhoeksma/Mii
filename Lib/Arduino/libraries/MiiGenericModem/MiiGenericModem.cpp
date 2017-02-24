#include <MiiGenericModem.h>

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


MiiGenericModem::MiiGenericModem(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
    :
    _mode(MII_RF_MODE_IDLE),
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
    _retryCount(MII_RF_DEFAULT_RETRY){
}

  //Power own the device
void MiiGenericModem::power(bool state){
  if (_sdnPin!=MII_NO_PIN) {
    //First turn off the modem
    pinMode(_sdnPin, OUTPUT);
    digitalWrite(_sdnPin, HIGH);
    delay(25);
    //Now Enable it again
    if (state) digitalWrite(_sdnPin, LOW);
  }
  delay(50);
}


bool MiiGenericModem::init(){
    //Default init procedure
    _ready=false;
    power(true);

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

    // Add by Adrien van den Bossche <vandenbo@univ-tlse2.fr> for Teensy
    // ARM M4 requires the below. else pin interrupt doesn't work properly.
    // On all other platforms, its innocuous, belt and braces
    pinMode(_intPin, INPUT);

    return true;
}

bool MiiGenericModem::recv(bool allowCollision,uint8_t* buf, uint8_t* len){
  bool ret = false;
  #if MII_MAX_COLLISIONS
   if (allowCollision && _collisionCount) ret=popCollision(buf,len);
  #endif
  if (!ret) { //We had no collision to process, see if there is other data
    ret = internalRecv(buf,len);
  }
  if (ret) {
     if (_rxHeaderFrom!=MII_BROADCAST_ADDRESS && _rxHeaderTo==_thisAddress &&
         (_rxHeaderFlags & MII_RF_FLAGS_ACK) && _rxTime!=_ackTime) {
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
    unsigned long starttime = millis();
    while (timeout==0 || (millis() - starttime) < timeout) {
        if (available(false))
           return true;
        YIELD;
    }
    return false;
}

bool MiiGenericModem::waitPacketSent(uint16_t timeout){
    unsigned long starttime = millis();
    while (timeout==0 || (millis() - starttime) < timeout) {
        if (_mode != MII_RF_MODE_TX) // Any previous transmit finished?
           return true;
       YIELD; // Wait for any previous transmit to finish
    }
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

void MiiGenericModem::setMaster(uint8_t address){
  _masterAddress=address;
  if (address==_thisAddress) {
    _timeSync=true;
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

void MiiGenericModem::setRetries(uint8_t retries){_retryCount=retries;} //Set default retry sendAckCmd
void MiiGenericModem::setTimeout(uint16_t timeout){_timeout=timeout;} //Set default timeout for WaitForCmd
void MiiGenericModem::setDrift(uint16_t drift){_drift=drift;}   //Set the time allowed to run appart in ms, before a full time sync will be done again
void MiiGenericModem::setTimeInterval(uint32_t interval){ _timeInterval = interval;} //Set the time update interval

//Just wait idle
void MiiGenericModem::idle(uint16_t timeout) {
  uint32_t t=timeout+millis();
  do {
   YIELD;
  } while (t>millis());
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
  #if SPI_ISOLATE
   SPDR = 0xFF;
   while (!(SPSR & (1 << SPIF)));
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
bool MiiGenericModem::discard(bool allowCollision){
 bool ret = false;
  #if MII_MAX_COLLISIONS
   if (allowCollision && _collisionCount) ret=popCollision();
  #endif
  if (!ret) { //We had no collision to process, see if there was normal data
    if (!hasAvailable()) return false; //We don't want any processing
    clearRxBuf();
    _rxHeaderTo=0;
    ret = true;
  }
  return ret;
}

//Send a command and wait for Ack when trys is set
bool  MiiGenericModem::sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t trys,uint8_t flags){
  bool ret=false;
  if (trys==0xFF) trys = _retryCount; //trys will be on more than number of retries
  if (trys) trys++; //We always loop so 1 try is always needed
  if (!address) address=_masterAddress; //When no address is specified we user master
  if (!address) return false; //We will not send data to a undefined address
  do  {
	  setHeaderFlags(trys && (address!=MII_BROADCAST_ADDRESS) ? (MII_RF_FLAGS_ACK | flags) : flags);
    setHeaderId(cmd);
    setHeaderTo(address);
    //When len is 0 we only send cmd as data
    ret = len> 0 ? send(buf, len) : send(&cmd,sizeof(cmd));
    waitPacketSent();

    if (ret && trys && address!=MII_BROADCAST_ADDRESS) {
     //Wait for ACK
     ret = waitForCmd(MII_C_ACK,0,0,address);
     //reduce the retry counts
     trys--;
    }
  } while (!ret && trys>0);

    //Check for master and if we should send update of time, not during sync
  if (_timeInterval && isMaster() && millis()>_lastTime+_timeInterval && cmd!=MII_C_TIMESYNC){
   // Serial.println("Sending master time");
   sendTime();
  }
  //Return to listening
  setModeRx();
  return ret;
}

//Send a command to a specific address
bool MiiGenericModem::sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t flags){
 return sendAckCmd(cmd,buf,len,address,0,flags);
}

bool MiiGenericModem::waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  if (!address) address=_masterAddress;  //We use master address for ok when not set
  uint32_t failtime=timeout+millis();
  //We wil not acknowledge messages
  while (failtime>millis()){
    if (available(false)) {
       if (_rxHeaderId==cmd && (address==MII_BROADCAST_ADDRESS || address==_rxHeaderFrom)) {
          return recv(false,buf,&len); //We are allowed to send ack we have received it
       }
      //We have a collision, received message we did not expect
      //Store it in collision stack if it is not full
      #if MII_MAX_COLLISIONS
      if (!pushCollision())  //No more room in collision stack, Clear message directly
      #endif
      recv(true);
    }
    YIELD;
  }
  return false;
}


#if MII_MAX_COLLISIONS
bool MiiGenericModem::pushCollision(){
 if (available(false) && _collisionCount<MII_MAX_COLLISIONS-1) {
    collisions[_collisionCount].headerId=_rxHeaderId;
    collisions[_collisionCount].headerFrom=_rxHeaderFrom;
    collisions[_collisionCount].headerTo=_rxHeaderTo;
    collisions[_collisionCount].headerFlags=(_rxHeaderFlags & ~_BV(MII_RF_FLAGS_ACK)); //We will ack below
    collisions[_collisionCount].length=MII_RF_MAX_MESSAGE_LEN;
    recv(false,collisions[_collisionCount].data,&collisions[_collisionCount].length); //We will allow ack to be send

    _collisionCount++; //We have read a new collision
    return true;
  }
  return false;
}

bool MiiGenericModem::popCollision(uint8_t* buf, uint8_t* len){
  if (_collisionCount) {
     if (buf && len){
       *len=min(*len,collisions[0].length);
       memmove(buf,collisions[0].data,*len);
      };
     //Check if we are in receiving mode and have already data in.
     //If so we have to move it in and out
      uint8_t rxHeaderId=collisions[0].headerId;
      uint8_t rxHeaderFrom=collisions[0].headerFrom;
      uint8_t rxHeaderTo=collisions[0].headerTo;
      uint8_t rxHeaderFlags=collisions[0].headerFlags;

      _collisionCount--;
      memmove(&collisions[0],&collisions[1],sizeof(collisionRec_t)*_collisionCount);
      if (available(false)) {
        //We have temporary save the collision header before swap record
        pushCollision(); //Add the available record to the stack

        } else { //Free up the collision
       }
      //We save record in collision so now we can swap headers
      _rxHeaderId=rxHeaderId;
      _rxHeaderFrom=rxHeaderFrom;
      _rxHeaderTo=rxHeaderTo;
      _rxHeaderFlags=rxHeaderFlags;
      return true;
  }
  return false;
}
#endif

bool MiiGenericModem::syncTime(uint8_t address){
   if (isMaster()) return false;
   _timeDiff=0; //Block sending data
   //Send a request for timesync to the MASTER, on receving the request Master will only accept messages from this device
   //Master will send his Time T[0] to this device will receive at T[1] and we will response with new request, skipped because of start master
   //Master will send his Time T[2] to this device will receive at T[3] and we will response with new request
   //Master will send his Time T[4] to this device will receive at T[5] and start calculation
   uint8_t _try=0;
   uint32_t T[6];
   _lastTime=millis();
   uint8_t toAddress=MII_BROADCAST_ADDRESS;
   for (uint8_t i=0;i<6;){
     if (!(sendCmd(MII_C_TIMESYNC,(uint8_t*)&address,sizeof(uint8_t)) &&
       waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),address)))
     { // Allow search for other time sync
      _try++;
      if (_try<4 && i==0) continue;
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
   _timeout=_airTime*2+MII_RF_MIN_MSG_INTERVAL*2;
   _lastTime=millis();
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

bool MiiGenericModem::internalProcess(bool allowCollision){
 bool ret = false;
 if (available(allowCollision)) {
   #if MII_MAX_ADDRESS
   //We have data build up the addressTable
   uint8_t a=0;
   for (a=0;a<_addressCount && _address[a].id!=_rxHeaderFrom;a++){}
   if (a<_addressCount) { //We found the record, shift first records to freeup record
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*a);
   } else { //New Record, insert at first place
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*_addressCount);
     if (_addressCount<MII_MAX_ADDRESS-1) _addressCount++;
   }
   //Fill the first record with data for recieved address
   _address[0].id=_rxHeaderFrom;
   _address[0].seen=millis(); //Create seconds
   _address[0].rssi=_lastRssi;

	 //Filter the messages, to see if they are for us
	 if (_rxHeaderFrom == _thisAddress ||
	    !(_rxHeaderTo == _thisAddress || _rxHeaderTo == MII_BROADCAST_ADDRESS)
	    ) { //Discard the message, it is not for us, no ack
	    discard(allowCollision);
	    return false;
	 }
	 #endif

   switch (_rxHeaderId){
    case MII_C_TIMESYNC :
       recv(allowCollision); //Clear TimeSync CMD
       if ( isMaster() || _rxHeaderTo==_thisAddress) { //Only master or direct requests will reply on Time Sync commands
         //Master will send 3 times data, no retry and will not wait for last reqst
         uint32_t t;
         for (uint8_t i=0;i<3;i++) {
           t=time();
           if (!sendCmd(MII_C_TIMESYNC,(uint8_t*)&t,sizeof(uint32_t),_rxHeaderFrom)) break;
           if (i==2 || !waitForCmd(MII_C_TIMESYNC,0,0,_rxHeaderFrom)) break; //Exit if did not send or got correct response
         }
       } else {
          //When somebody is doing time sync make sure we don't send a message
          _txTime=millis()+_airTime*2;
          _lastTime=max(_lastTime,_txTime);
       }
       break;

   case MII_C_REQ_TIME:
      recv(allowCollision); //Clear Request Time CMD
      if (isMaster()) sendTime();
      break;

   #if MII_C_CHANNEL
   case MII_C_CHANNEL: {
         uint8_t channel;
         if (recv(allowCollision,&channel,sizeof(uint8_t)) changeChannel(channel);
    } break;
    #endif

    case MII_C_TIME:
        if (!isMaster()) { //We are just a client and should process this time from master
           uint32_t _time;
           uint8_t _len=sizeof(_time);
           if (recv(allowCollision,(uint8_t *)&_time,&_len) && _time) { //Only accept time if it is set
             long diff = time(_rxTime)-_airTime-_time;
             _lastTime=_rxTime;
             #if MIIRF_SERIAL >= 2
              Serial.print("Time:");Serial.print(time(_rxTime));Serial.print("-");Serial.print(time(_airTime));Serial.print("-");Serial.print(_time);Serial.print("=");Serial.print(diff);Serial.print('<');Serial.print(_drift);Serial.print(" [");Serial.print(millis());Serial.print('-');Serial.print(_rxTime);Serial.println("]");Serial.flush();
             #endif
              if (!_timeSync) {
               _masterAddress=_rxHeaderFrom; //Valid time so set the masterAddress
               _timeDiff-=diff;
               _lastTime=millis();
              //We balance the time drift by adding half of the drift if difference is within 10ms
              } else if (abs(diff)<=_drift && _timeDiff) { //We need to valid timediff before we calculate diff
                 _timeDiff-=((diff*3)/4); //For bigger drifts gap is closed better (.75)
                 _lastTime=millis();
              } else { //When difference to large request timing
                syncTime();
              }
           }
        } else {
          discard(allowCollision); //Clear Time CMD, without ack
        }
        break;
    default : ret=true; break; //We have not internal processed so it is available
    }
  } else { //Now data was available to process
    //  check if we should do time sync, when never registered do it fast
    if (_timeInterval && !isMaster() && (_lastTime+(_timeDiff==0 ? _timeInterval/3 : _timeInterval))<millis()){
     if (!_timeSync) {
        if (_timeDiff==0) sendCmd(MII_C_REQ_TIME);
        _lastTime=millis();
      } else
       syncTime();
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

bool MiiGenericModem::available(bool allowCollision){
  //If we allow collision the be retrieved, check if we have them available
   #if MII_MAX_COLLISIONS
  if (allowCollision && _collisionCount) {
     pushCollision(); //If there is real data just pusch it on the stack
     //Fill the headers with the data available
     _rxHeaderId=collisions[0].headerId;
     _rxHeaderFrom=collisions[0].headerFrom;
     _rxHeaderTo=collisions[0].headerTo;
     _rxHeaderFlags=collisions[0].headerFlags;
     return true;
  }
  #endif

  //If where are in the internal processing loops return the original availability
  if (_inAvailable) return hasAvailable();

  _inAvailable=true;
  bool ret = internalProcess(allowCollision);
  //Unlock the available loop
  _inAvailable=false;
  if (!ret) {
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
uint8_t MiiGenericModem::nextAddress(uint8_t address,uint32_t liveSpan){
  if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list and our own address
  uint8_t ret=_thisAddress>address ? _thisAddress :0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id>address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiGenericModem::prevAddress(uint8_t address,uint32_t liveSpan){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : max(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiGenericModem::firstAddress(uint8_t address,uint32_t liveSpan){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}
#endif
