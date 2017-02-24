#include <MiiRF.h>

//Set MIIRF_SERIAL to 1 if you need serial debugging, 2 if time sync is needed
#define MIIRF_SERIAL 0

MiiRF::MiiRF(uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t sdnPin, RHGenericSPI& spi) :
#if defined(RH_RF22_h)
 MII_RF_EXT (slaveSelectPin,interruptPin,spi)
#elif defined(RH_RF24_h)
 MII_RF_EXT (slaveSelectPin,interruptPin,sdnPin,spi)
#endif
{
  _airTime=MII_RF_MIN_MSG_INTERVAL;
  _timeout=MII_RF_DEF_TIMEOUT;
  _retryCount=MII_RF_DEFAULT_RETRY;
  _drift=MII_RF_DEFAULT_DRIFT;
  #if  defined(RH_RF22_h)
  _sdnPin=sdnPin;
  #endif
}

bool MiiRF::send(const uint8_t* data, uint8_t len){
 idle((_txTime + _airTime)-millis()); //Make sure we send message in correct time seq
 _txTime=millis();
 return MII_RF_EXT::send(data,len);
}

#ifdef RH_RF22_h
void MiiRF::power_on_reset(){
    if (_sdnPin!=0xff) {
      // Sigh: its necessary to control the SDN pin to reset this ship.
      // Tying it to GND does not produce reliable startups
      // Per Si4464 Data Sheet 3.3.2
      digitalWrite(_sdnPin, HIGH); // So we dont get a glitch after setting pinMode OUTPUT
      pinMode(_sdnPin, OUTPUT);
      delay(10);
      digitalWrite(_sdnPin, LOW);
      delay(10);
    }
    #if MII_MAX_COLLISIONS
    _collisionCount=0;
    #endif
}
#endif

bool  MiiRF::init(){
 power_on_reset();
 return MII_RF_EXT::init();
}

/// A more easy init procedure
bool MiiRF::init(uint8_t address,uint16_t syncWords,uint8_t channel,uint8_t power,bool isMaster,ModemConfigChoice index){
  power_on_reset();
  if (MII_RF_EXT::init()){
    setAddress(address);
    setSyncWords(syncWords);
    setChannel(channel,power,index);
    if (isMaster) setMaster(address);
    return true;
  }
  return false;
}


//Just wait idle
void MiiRF::idle(long time) {
  if (time<=0){
    YIELD;
    return;
  }
  uint32_t t=time+millis();
  do {
   YIELD;
  } while (t>millis());
}

#if MII_MAX_COLLISIONS
bool MiiRF::pushCollision(){
 if (MII_RF_EXT::available() && _collisionCount<MII_MAX_COLLISIONS-1) {
    #if MIIRF_SERIAL
     Serial.println("COL PUSH: DATA");
    #endif
    collisions[_collisionCount].headerId=_rxHeaderId;
    collisions[_collisionCount].headerFrom=_rxHeaderFrom;
    collisions[_collisionCount].headerTo=_rxHeaderTo;
    collisions[_collisionCount].headerFlags=(_rxHeaderFlags & ~_BV(MII_RF_FLAGS_ACK)); //We will ack below
    collisions[_collisionCount].length=MII_RF_MAX_MESSAGE_LEN;
    recv(false,collisions[_collisionCount].data,&collisions[_collisionCount].length); //We will allow ack to be send

    _collisionCount++; //We have read a new collision
    return true;
  } else if (_collisionCount<MII_MAX_COLLISIONS-1) setModeIdle(); //Return to idle mode
  return false;
}

bool MiiRF::popCollision(uint8_t* buf, uint8_t* len){
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
    if (MII_RF_EXT::available()) {
      #if MIIRF_SERIAL
      Serial.println(F("COL POP: WITH DATA IN"));
      #endif
      //We have temporary save the collision header before swap record
      pushCollision(); //Add the available record to the stack

    } else { //Free up the collision
     #if MIIRF_SERIAL
     Serial.println(F("COL POP: WITHOUT DATA IN"));
     #endif
     setModeIdle(); //Return to idle mode
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

//Wrapper for the old recieved
bool MiiRF::recv(uint8_t* buf, uint8_t* len){
 return recv(true,buf,len);
}

bool MiiRF::recv(bool allowCollision,uint8_t* buf, uint8_t* len){
  bool ret = false;
   #if MII_MAX_COLLISIONS
  if (allowCollision && _collisionCount) ret=popCollision(buf,len);
  #endif
  if (!ret) { //We had no collision to process, see if there is other data
    ret = MII_RF_EXT::recv(buf, len);
  }
  if (ret) {
     if (_rxHeaderFrom!=MII_BROADCAST_ADDRESS && _rxHeaderTo==_thisAddress && (_rxHeaderFlags & MII_RF_FLAGS_ACK) && _rxTime!=_ackTime) {
       #if MIIRF_SERIAL
       Serial.print("Sending Ack:");Serial.print(_rxHeaderId);Serial.print(' ');Serial.println(_rxHeaderFrom);Serial.flush();
       #endif
       _ackTime=_rxTime;
       setHeaderId(MII_C_ACK);
       setHeaderFlags(0,0xFF);
       setHeaderTo(_rxHeaderFrom);
       send((uint8_t*)&_rxHeaderId, sizeof(_rxHeaderId));
       waitPacketSent();
    }
  }
  return ret;
}

bool MiiRF::recv8_t(uint8_t  &buf,bool allowCollision){uint8_t len=sizeof(uint8_t);return recv(allowCollision,(uint8_t*)&buf,&len);}
bool MiiRF::recv16_t(uint16_t  &buf,bool allowCollision){uint8_t len=sizeof(uint16_t);return recv(allowCollision,(uint8_t*)&buf,&len);}
bool MiiRF::recv32_t(uint32_t  &buf,bool allowCollision){uint8_t len=sizeof(uint32_t);return recv(allowCollision,(uint8_t*)&buf,&len);}

bool MiiRF::clearRecv(bool allowCollision){
  bool ret = false;
   #if MII_MAX_COLLISIONS
  if (allowCollision && _collisionCount) ret=popCollision();
  #endif
  if (!ret) { //We had no collision to process, see if there is other data
    ret = MII_RF_EXT::recv(NULL,0);
  }
  return ret;
}

bool MiiRF::syncTime(uint8_t address){
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
     if (!(sendCmd(MII_C_TIMESYNC,address) &&
       waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),address)))
     { // Allow search for other time sync
      _try++;
      if (_try<4 && i==0) continue;
      #if MIIRF_SERIAL >= 2
          Serial.println("Failed sync");Serial.flush();
      #endif
      return false;

     }
    // Serial.print("T[");Serial.print(i);Serial.print("]=");Serial.print(T[i]);Serial.print(" T[");Serial.print(i+1);Serial.print("]=");Serial.println(_rxTime);Serial.flush();
     i++; //The send time
     if (address==MII_BROADCAST_ADDRESS) address=_rxHeaderFrom; //toAddress can be broadcast for next rounds we take the real address
     T[i++]=_rxTime; //The receive time
   } //For
   _masterAddress=address; //Valid time so set the masterAddress
   //Calculate transmit time by dividing round trip using the two times (T[2] and T[4]) of master by 2
   //Add this round trip to the T[4]send time of master and substract the receive time T[5]
   _airTime=(T[4]-T[2])/2;
   _timeDiff=T[4]+_airTime-T[5];
   #if MIIRF_SERIAL >= 2
     Serial.print("New Diff:(");Serial.print(_airTime);Serial.print(')');Serial.println(_timeDiff);Serial.flush();
   #endif
    _lastTime=millis();
   return true;
}

bool MiiRF::internalProcess(bool allowCollision){
 boolean ret = false;
 if (available(allowCollision)) {
   #if MIIRF_SERIAL >= 1
     Serial.print("New:");Serial.print(_rxHeaderFrom);Serial.print(' ');Serial.println(_rxHeaderId);Serial.flush();
   #endif
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
   _address[0].rssi=lastRssi();

   //Filter the messages, to if they are for us
	 if (!(_rxHeaderTo == _thisAddress ||
	    _rxHeaderTo == MII_BROADCAST_ADDRESS)) { //Discard the message, it is not for us
	    clearRecv(allowCollision);
	    return false;
	 }
	 #endif

   switch (_rxHeaderId){
    case MII_C_TIMESYNC :
       recv(allowCollision); //Clear TimeSync CMD
       if ( isMaster() || _rxHeaderTo==_thisAddress) { //Only master or direct requests will reply on Time Sync commands
         //Master will send 3 times data, no retry and will not wait for last reqst
         for (uint8_t i=0;i<3;i++) {
 //           Serial.print("Sending time ");Serial.println(time());Serial.flush();
           if (!sendCmd(MII_C_TIMESYNC,time(),_rxHeaderFrom)) break;
           if (i==2 || !waitForCmd(MII_C_TIMESYNC,_rxHeaderFrom)) break; //Exit if did not send or got correct response
         }
       } else {
         setModeIdle(); //Receive no info
         idle(_airTime*6); //When we are not doing time sync make sure we don't send a message
       }
       break;

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
              if (noTimeSync) {
               _masterAddress=_rxHeaderFrom; //Valid time so set the masterAddress
               _timeDiff-=diff;
              //We balance the time drift by adding half of the drift if difference is within 10ms
              } else if (abs(diff)<=_drift && _timeDiff) { //We need to valid timediff before we calculate diff
                   _timeDiff-=((diff*3)/4); //For bigger drifts gap is closed better (.75)
              } else { //When difference to large request timing
                syncTime();
              }
           }
        } else {
          recv(allowCollision); //Clear Time CMD, without any ack
        }
        break;
    default : ret=true; break; //We have not internal processed so it is available
    }
  } else { //Now data was available to process
    //  check if we should do time sync, when never registered do it fast
    if (!noTimeSync && _timeInterval && !isMaster() && (_lastTime+(_timeDiff==0 ? _timeInterval/3 : _timeInterval))<millis()){
       #if MIIRF_SERIAL >= 2
         Serial.print("Request NEW TIME:");Serial.print(_timeInterval);Serial.print('+');Serial.print(_lastTime);Serial.print('<');Serial.println(millis());
       #endif
      syncTime();
    }
  }
  return ret;
}


//Relay orignal available to the new one
bool MiiRF::available(){
  return available(true);
}

bool MiiRF::available(bool allowCollision){
  //If we allow collision the be retrieved, check if we have them available
   #if MII_MAX_COLLISIONS
  if (allowCollision && _collisionCount) {
     pushCollision(); //If there is real data just pusch it on the stack
     #if MIIRF_SERIAL
      Serial.println(F("COL AV: DATA"));
     #endif
     //Fill the headers with the data available
     _rxHeaderId=collisions[0].headerId;
     _rxHeaderFrom=collisions[0].headerFrom;
     _rxHeaderTo=collisions[0].headerTo;
     _rxHeaderFlags=collisions[0].headerFlags;
     return true;
  }
  #endif

  //If where are in the internal processing loops return the original availability
  if (_inAvailable) return MII_RF_EXT::available();

  _inAvailable=true;
  boolean ret = internalProcess(allowCollision);
  //Unlock the available loop
  _inAvailable=false;
  if (!ret) idle();
  return ret;
}


//Send a command and wait for Ack when trys is set
bool  MiiRF::sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t trys,uint8_t flags){
  boolean ret=false;
  if (trys==0xFF) trys = _retryCount; //trys will be on more than number of retries
  if (trys) trys++; //We always loop so 1 try is always needed
  if (!address) address=_masterAddress; //When no address is specified we user master
  #if MIIRF_SERIAL >= 2
  Serial.print("Send CMD:");Serial.print(cmd);Serial.print(' ');Serial.print(trys);Serial.print(' ');Serial.print(address);Serial.print(' ');Serial.print(flags);Serial.print(' ');Serial.println(len);Serial.flush();
  #endif
do  {
	setHeaderFlags(trys && (address!=MII_BROADCAST_ADDRESS) ? (MII_RF_FLAGS_ACK | flags) : flags,0xFF);
  setHeaderId(cmd);
  setHeaderTo(address);
  //When len is 0 we only send cmd as data
  ret = len> 0 ? send(buf, len) : send(&cmd,sizeof(cmd));
  #if MIIRF_SERIAL >= 2
    Serial.print("Send: ");Serial.println(len);Serial.flush();
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
  }
} while (!ret && trys>0);

    //Check for master and if we should send update of time, not during sync
  if (_timeInterval && isMaster() && millis()>_lastTime+_timeInterval && cmd!=MII_C_TIMESYNC){
   // Serial.println("Sending master time");
   // idle(_airTime);
  	setHeaderFlags(0,0xFF);
    setHeaderId(MII_C_TIME);
    setHeaderTo(MII_BROADCAST_ADDRESS);
    uint32_t _time = time();
    if (send((uint8_t*)&_time,sizeof(_time))) {
       _lastTime=millis();
       waitPacketSent();
    }
  }
  return ret;
}

bool MiiRF::sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,trys,flags);
}

bool  MiiRF::sendAckCmd(uint8_t cmd,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,NULL,0,address,trys,flags);
}

//Send a command to a specific address
bool MiiRF::sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t flags){
 return sendAckCmd(cmd,buf,len,address,0,flags);
}

bool MiiRF::sendCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,0,flags);
}

bool MiiRF::sendCmd(uint8_t cmd,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,NULL,0,address,0,flags);
}


bool MiiRF::waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  if (!address) address=_masterAddress;  //We use master address for ok when not set
  uint32_t failtime=timeout+millis();
  //We wil not acknowledge messages
  while (failtime>millis()){
    if (available(false)) {
       if (_rxHeaderId==cmd && (address==MII_BROADCAST_ADDRESS || address==_rxHeaderFrom)) {
          #if MIIRF_SERIAL
           Serial.print(F("Wait Found:"));Serial.println(cmd);
          #endif
          return recv(false,buf,&len); //We are allowed to send ack we have received it
       }
      #if MIIRF_SERIAL
           Serial.print(F("Wait got:"));Serial.println(cmd);
       #endif
      //We have a collision, received message we did not expect
      //Store it in collision stack if it is not full
      #if MII_MAX_COLLISIONS
      if (!pushCollision())  //No more room in collision stack, Clear message directly
      #endif
        MII_RF_EXT::recv(NULL,0);
    }
    idle();
  }
  return false;
}

//Wait for a empty command
bool MiiRF::waitForCmd(uint8_t cmd,uint8_t address,uint16_t timeout){
  return waitForCmd(cmd,0,0,address,timeout);
}


uint32_t MiiRF::time(uint32_t _time){
 return isMaster() ? (_time ? _time : millis()) : (_timeDiff ? (_time ? _time : millis())+ _timeDiff : _time );
}

bool MiiRF::isMaster(){
  return _masterAddress==_thisAddress;
}

void MiiRF::setSyncWords(const uint16_t syncWords){
 MII_RF_EXT::setSyncWords((const uint8_t*)&syncWords, 2);
}

/* European lookup table for 433 Mhz
static rf_channels []
1 	433.075 	24 	433.650 	47 	434.225
2 	433.100 	25 	433.675 	48 	434.250
3 	433.125 	26 	433.700 	49 	434.275
4 	433.150 	27 	433.725 	50 	434.300
5 	433.175 	28 	433.750 	51 	434.325
6 	433.200 	29 	433.775 	52 	434.350
7 	433.225 	30 	433.800 	53 	434.375
8 	433.250 	31 	433.825 	54 	434.400
9 	433.275 	32 	433.850 	55 	434.425
10 	433.300 	33 	433.875 	56 	434.450
11 	433.325 	34 	433.900 	57 	434.475
12 	433.350 	35 	433.925 	58 	434.500
13 	433.375 	36 	433.950 	59 	434.525
14 	433.400 	37 	433.975 	60 	434.550
15 	433.425 	38 	434.000 	61 	434.575
16 	433.450 	39 	434.025 	62 	434.600
17 	433.475 	40 	434.050 	63 	434.625
18 	433.500 	41 	434.075 	64 	434.650
19 	433.525 	42 	434.100 	65 	434.675
20 	433.550 	43 	434.125 	66 	434.700
21 	433.575 	44 	434.150 	67 	434.725
22 	433.600 	45 	434.175 	68 	434.750
23 	433.625 	46 	434.200 	69 	434.775
*/
void MiiRF::setChannel(uint8_t channel,uint8_t power,ModemConfigChoice index){
  float freq=433.050 + (channel*0.025);
  setModemConfig(index);
  setFrequency(freq, 0.0125);
  setTxPower(power);
}

void MiiRF::setAddress(uint8_t address){
  setThisAddress(address);
  setHeaderFrom(address);
  #if MII_MAX_ADDRESS
  setPromiscuous(true); //We will filter the address ourself in available
  #endif
  if (!_timeInterval) _timeInterval=MII_TIME_INTERVAL_CLIENT;
}

void MiiRF::setMaster(uint8_t address){
  _masterAddress=address;
  if (_timeInterval==MII_TIME_INTERVAL_CLIENT) _timeInterval=MII_TIME_INTERVAL_MASTER;
}

#if MII_MAX_ADDRESS
uint8_t MiiRF::nextAddress(uint8_t address,uint32_t liveSpan){
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

uint8_t MiiRF::prevAddress(uint8_t address,uint32_t liveSpan){
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

uint8_t MiiRF::firstAddress(uint8_t address,uint32_t liveSpan){
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