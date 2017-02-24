#include <MiiRF22.h>

// via bit-bang software instead of a hardware SPI driver
#if RF22_SOFT_SPI == 1
#include <SoftwareSPI.h>
SoftwareSPIClass Software_spi;
#else
#include <SPI.h>
#endif

#include <util/atomic.h>
#define ATOMIC_BLOCK_START     ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#define ATOMIC_BLOCK_END }

MiiRF22::MiiRF22(uint8_t thisAddress, uint8_t slaveSelectPin, uint8_t interrupt,uint16_t drift):
    RF22(slaveSelectPin, interrupt){
  // Configure SoftwareSPI for the same pins as usual hardware
#if RF22_SOFT_SPI == 1
  Software_spi.setPins(12, 11, 13);
   _spi = &Software_spi;
#endif
  _thisAddress=thisAddress;
  _masterAddress=0;
  _rxTime=0;
  _drift=drift;
  setRetries();
  setTimeout();
}

boolean MiiRF22::init(boolean isMaster){
    boolean ret = this->RF22::init();
    if (ret)  {
      setAddress(_thisAddress);
      if (isMaster) setMaster(_thisAddress);
    }
    return ret;
}

void MiiRF22::setAddress(uint8_t thisAddress){
    _thisAddress = thisAddress;
    spiWrite(RF22_REG_3F_CHECK_HEADER3, _thisAddress );
    // Use this address in the transmitted FROM header
    setHeaderFrom(_thisAddress);
}

uint8_t MiiRF22::getMasterAddress(){
  return _masterAddress;
}

//Send a command and wait for Ack when trys is set
boolean  MiiRF22::sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t trys,uint8_t flags){
  boolean ret;
  if (trys==0xFF) trys = _retryCount+1; //trys will be on more than number of retries
  if (!address) address=_masterAddress; //When no address is specified we user master

  #if MII_RF22_ID
  //Check if we should add a routing id to the message flags
  if (!flags || ((flags & RF22_FLAGS_ID) && !(flags & 0x0F))){
    _msgId++;
    if (_msgId>0x0F) _msgId=1;
    flags|=(_msgId | RF22_FLAGS_ID);
  }
  #endif


retrySend:
	setHeaderFlags(trys && address!=MII_BROADCAST ? RF22_FLAGS_CON | flags : flags);
  setHeaderId(cmd);
  setHeaderTo(address);
  //When len is 0 we only send cmd as data
  ret = len> 0 ? send(buf, len) : send(&cmd,sizeof(cmd));
  waitPacketSent();

  if (ret && trys && address!=MII_BROADCAST) {
     //Wait for ACK
     uint32_t timeout=millis()+_timeout;
     while (timeout>millis()) {
       if (available()) {
         if ((headerFlags() & RF22_FLAGS_ACK) && headerId()==cmd && headerFrom()==address) {
           clearRxBuf(); // Not using recv, so clear it ourselves
           ret=true;
           goto exitSend;
         }
         clearRxBuf(); // Not using recv, so clear it ourselves
       }
     }
     //retransmit
     trys--;
     if (trys) goto retrySend;
     ret=false;
  }
exitSend:
    //Check for master and if we should send update of time
  if (isMaster() && millis()>_lastTime && cmd!=MII_C_TIMESYNC){
    wait(RF_MIN_MESSAGE_INTERVAL);
  	setHeaderFlags(0);
    setHeaderId(MII_C_TIME);
    setHeaderTo(MII_BROADCAST);
    uint32_t _time = time();
    if (send((uint8_t*)&_time,sizeof(_time))) {
       _lastTime=millis()+MII_LASTTIME_INTERVAL_SEND;
       waitPacketSent();
    }
  }
  return ret;
}

boolean  MiiRF22::sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(buf),address,trys,flags);
}

boolean  MiiRF22::sendAckCmd(uint8_t cmd,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,NULL,0,address,trys,flags);
}

//Send a command to a specific address
boolean MiiRF22::sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t flags){
 return sendAckCmd(cmd,buf,len,address,0,flags);
}

boolean MiiRF22::sendCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(buf),address,0,flags);
}

boolean MiiRF22::sendCmd(uint8_t cmd,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,NULL,0,address,0,flags);
}

int MiiRF22::waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  uint32_t failtime=timeout+millis();
  uint8_t _len=len==0 ? 1 : len; //Min length = 1;
  //We wil not acknowledge messages
  while (failtime>millis()){
    if (this->RF22::available() && recv(buf,&_len) &&
       headerId()==cmd && (address==MII_BROADCAST || address==headerFrom())) return _len;
    idle();
  }
  return 0;
}

//Wait for a empty command
boolean MiiRF22::waitForCmd(uint8_t cmd,uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  uint8_t _buf;
  uint8_t _len=sizeof(_buf);
  uint32_t failtime=timeout+millis();
  //We wil not acknowledge messages
  while (failtime>millis()){
    if (this->RF22::available() && recv(&_buf,&_len) &&
       headerId()==cmd && (address==MII_BROADCAST || address==headerFrom())) return _buf==cmd;
    idle();
  }
  return false;
}

//avaliable will do internal processing of time and send out interval if needed
boolean MiiRF22::available(){
  boolean ret = this->RF22::available();
  //We only do special function if message was specific for us
  if (_isPromiscuous || vbi(_lock,RF22_LOCK_AVAILABLE)) {
    if (!ret) idle();
    return ret;
  }
  sbi(_lock,RF22_LOCK_AVAILABLE);
  if (ret) {

   //Check if we need to confirm message send to us and prevent ack of same message twice
   if (_rxHeaderTo==_thisAddress && (_rxHeaderFlags & RF22_FLAGS_CON) && _rxTime!=_ackTime) {
     _ackTime=_rxTime;
     setHeaderId(_rxHeaderId);
     setHeaderFlags(RF22_FLAGS_ACK);
     setHeaderTo(_rxHeaderFrom);
     //We need to keep send buffer for further use, so just send just first byte back
     uint8_t  bufLen;
     ATOMIC_BLOCK_START;
     bufLen = _bufLen;
     _bufLen=1;
     _txBufSentIndex = 0;
     startTransmit();
     ATOMIC_BLOCK_END;
     waitPacketSent();
     //Restore orignal received data len;
     ATOMIC_BLOCK_START;
     _bufLen=bufLen;
     ATOMIC_BLOCK_END;
   }

   switch (_rxHeaderId){
    case MII_C_TIMESYNC_SEARCH:
      if (time()) { //Check if we have a time
         if (!isMaster()) wait(RF_MIN_MESSAGE_INTERVAL+_thisAddress/10); //Delay so we don't colide with others
         _rxHeaderTo=_thisAddress; //Force send in time sync by changing _rxHeaderTo
       } else {
        clearRxBuf();
        break; //We need to have a time if we want to be a link
       }
       //Fall trough to TimeSync
    case MII_C_TIMESYNC :
       clearRxBuf(); //Clear TimeSync CMD
       ret=false;
       if ( isMaster() || _rxHeaderTo==_thisAddress) { //Only master or direct requests will reply on Time Sync commands
         //Master will send 3 times data, no retry and will not wait for last reqst
         for (uint8_t i=0;i<3;i++) {
           if (!sendCmd(MII_C_TIMESYNC,time(),_rxHeaderFrom)) break;
           if (i==2 || !waitForCmd(MII_C_TIMESYNC,_rxHeaderFrom)) break; //Exit if did not send or got correct response
         }
       } else {
         wait(RF_MIN_MESSAGE_INTERVAL*6); //When we are not doing time sync make sure we don't send a message
       }
       break;

    case MII_C_TIME:
        ret=false;
        if (!isMaster()) { //We are just a client and should process this time from master
        //   uint32_t _time;
           uint8_t _len=sizeof(_time);
           if (recv((uint8_t *)&_time,&_len) && _time) { //Only accept time if it is set
             long diff = time(_rxTime)-_airTime-_time;
 //             Serial.println();Serial.print("Time:");Serial.print(time(_rxTime));Serial.print("-");Serial.print(time(_airTime));Serial.print("-");Serial.print(_time);Serial.print("=");Serial.print(diff);Serial.print(" [");Serial.print(millis());Serial.print('-');Serial.print(_rxTime);Serial.println("]");Serial.flush();
              //We balance the time drift by adding half of the drift if difference is within 10ms
              if (abs(diff)<=_drift && _timeDiff) { //We need to valid timediff before we calculate diff
                 //_timeDiff-=diff/2;
                   _timeDiff-=((diff*3)/4); //For bigger drifts gap is closed better (.75)
              } else { //When difference to large request timing
                _timeDiff=0; //Block sending data
                goto availableTimeDiff;
              }
           } else {
//           Serial.println();Serial.println("Failed Recieve Time:");Serial.flush();
           }
        } else {
//          Serial.println();Serial.println("Ignore Time:");Serial.flush();
          clearRxBuf(); //Clear Time CMD
        }

        break;

    }

  }

  //No data check if we should synctime with master (only if we received message)
  if (_rxTime!=0 && !ret && !isMaster() && !_timeDiff && _lastTime+MII_LASTTIME_INTERVAL_RECV<millis() ) {
availableTimeDiff:
      _lastTime=millis();
      _syncCount++;
     //Send a request for timesync to the MASTER, on receving the request Master will only accept messages from this device
     //Master will send his Time T[0] to this device will receive at T[1] and we will response with new request, skipped because of start master
     //Master will send his Time T[2] to this device will receive at T[3] and we will response with new request
     //Master will send his Time T[4] to this device will receive at T[5] and start calculation
     uint8_t _try=0;
     uint8_t toAddress=MII_BROADCAST;
     for (uint8_t i=0;i<6;){
      if (!(sendCmd(MII_C_TIMESYNC,toAddress) &&
          waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),toAddress)==sizeof(uint32_t))) {
          // Allow search for other time sync
        //   if (i!=0 || !(sendCmd(MII_C_TIMESYNC_SEARCH,toAddress) && waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),toAddress)==sizeof(uint32_t)))
 //              Serial.println();Serial.print("Failed Diff:");Serial.print(i);Serial.print('-');Serial.print(_try);Serial.println("");Serial.flush();
              _try++;
              if (_try<4 && i==0) continue;
              goto AvailableExit;

       }
       i++; //The send time
       if (toAddress==MII_BROADCAST) toAddress=headerFrom(); //toAddress can be broadcast for next rounds we take the real address
       T[i++]=_rxTime; //The receive time
     }
     _masterAddress=toAddress; //Valid time so set the masterAddress
     //Calculate transmit time by dividing round trip using the two times (T[2] and T[4]) of master by 2
     //Add this round trip to the T[4]send time of master and substract the receive time T[5]
    _airTime=(T[4]-T[2])/2;
    _timeDiff=T[4]+_airTime-T[5];
//    Serial.println();Serial.print("New Diff:");;Serial.print(_timeDiff);Serial.println("");Serial.flush();

  //Check if we as master should broadcast time, only if we did not receive data for some time
  }
AvailableExit:
  cbi(_lock,RF22_LOCK_AVAILABLE);
  if (!ret) idle();
  return ret;
}

bool MiiRF22::recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    if (recv(buf, len))
    {
	if (from)  *from =  _rxHeaderFrom;
	if (to)    *to =    _rxHeaderTo;
	if (id)    *id =    _rxHeaderId;
	if (flags) *flags = _rxHeaderFlags;
	return true;
    }
    return false;
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
void MiiRF22::setChannel(uint8_t channel,uint8_t power,ModemConfigChoice index){
  float freq=433.050 + channel*0.025;
  setModemConfig(index);
  setTxPower(power);
  setFrequency(freq, 0.0125);
}

void MiiRF22::setSyncWords(const uint16_t syncWords){
 _syncwords=syncWords;
 spiBurstWrite(RF22_REG_36_SYNC_WORD3, (const uint8_t*)&_syncwords, 2);
}



