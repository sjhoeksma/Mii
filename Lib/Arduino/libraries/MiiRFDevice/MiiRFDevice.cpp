#include <MiiRFDevice.h>

 ///Init the based independent of RF device used
  MiiRFDevice::MiiRFDevice(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
  : MII_MODEM_CLASS(selectPin,intPin,sdnPin) {
   setStartDelay(MII_DEFAULT_START_DELAY);
   clearData();
}

uint8_t MiiRFDevice::updateActiveCount(){
  //Update the active count
  uint8_t count=0;
  for (uint8_t pos=0;pos<MAX_TIMES;pos++){
    if (timing.times[pos].start && timing.times[pos].state<MII_STATE_FINISH) {
      count++;
    }
  }
  timing.mode=(timing.mode & 0xF0) | (count);  //Current Mode + the count
  return count;
}

bool MiiRFDevice::getTimeSync(){
  return (timing.mode & 0x0F) ? false : _timeSync;
}

bool MiiRFDevice::addTime(uint32_t time,uint8_t sourceAddress) {
  if (!isMaster()) {
    bool ack=false;
    if (sendAckCmd(MII_CMD_SWITCH,time)) {
        waitForCmd(MII_CMD_SWITCH_ACK,(uint8_t*)&ack,sizeof(bool));
    }
    return ack;
  } else {
    sendCmd(MII_CMD_SWITCH,time); //Just inform all other clients about the switch of master
    if (addTiming(time,sourceAddress))
       return sendTiming();
  }
}

bool MiiRFDevice::addTiming(uint32_t time,uint8_t sourceAddress) {
    //Find the lastClient in the active list
    int pos=0;
    uint8_t lastClient,firstClient,clientCount;
    firstLastAddress(firstClient,lastClient,clientCount);
    //Get the prevAddress, keeping in account that we have open time so change livespan
    uint8_t prevClient=prevAddress(lastClient);
//    Serial.print("AddTime: ");Serial.print(timing.mode);Serial.print(' '); Serial.print(firstClient);Serial.print(' ');Serial.print(lastClient);Serial.print(' ');Serial.print(prevClient);Serial.print(' '); Serial.println(sourceAddress);

    //We are not accepting switches from devices after finish
    if (sourceAddress > lastClient || !(timing.mode & (MII_MODE_TIME | MII_MODE_PARALLEL))) return false;

    uint8_t updateState=(sourceAddress==lastClient ? MII_STATE_FINISH : sourceAddress);
    //Change the finish state if we are in parallel mode
    if ((sourceAddress==prevClient || sourceAddress==lastClient) && (timing.mode & MII_MODE_PARALLEL)) {
      //If Parallel mode need to finish left or right first
      //See if there is a record
      pos=MAX_TIMES-1;
      for (;pos>=0 && (!timing.times[pos].start || timing.times[pos].state>=MII_STATE_FINISH);pos--){}
      if (pos>=0) {
         if (timing.times[pos].state==prevClient && sourceAddress==lastClient ) {
           //Normal close, previous lane wins, time is set as difference, or if negative switch lane
           long tDiff=(time-timing.times[pos].start)-timing.times[pos].time;
           timing.times[pos].state=tDiff>=0 ? MII_STATE_FINISH_PREV : MII_STATE_FINISH_LAST ;
           timing.times[pos].time=abs(tDiff);
         } else if (timing.times[pos].state==lastClient && sourceAddress==prevClient) {
          //Inverse close, last lane wins,time is set as difference, or if negative switch lane
           long tDiff=(time-timing.times[pos].start)-timing.times[pos].time;
           timing.times[pos].state=tDiff>=0 ? MII_STATE_FINISH_LAST : MII_STATE_FINISH_PREV ;
           timing.times[pos].time=abs(tDiff);
         } else {
          //normal update, just one of them received
          timing.times[pos].state=sourceAddress;
          timing.times[pos].time=(timing.times[pos].start>time) ? 0 : time-timing.times[pos].start; //Keep in mind possible DRIFT, causing a negative
         }
         goto addTime_exit;
      }
       //Not found check if we should add, with 1 devices connected we are allowed to used both as start
       if (sourceAddress==firstClient || (clientCount==1 && sourceAddress==nextAddress(firstClient)) ) {
         updateState=sourceAddress;
         goto addTime_new; //Not found add
       }
 //      Serial.println(F("Invalid add"));
       return false; //Not valid to add
    }
    if (sourceAddress==firstClient) { //New start time
        if (sourceAddress==lastClient) return false; //We will not add if only one client
 addTime_new:
        //Check if we have as start for a already closed finish
       if (timing.times[0].state>=MII_STATE_FINISH && !timing.times[0].time && time-_drift<=timing.times[0].start) {
         timing.times[0].time=(timing.times[0].start>time) ? 0 : time-timing.times[0].start; //Keep in mind possible DRIFT, causing a negative
       } else {
         if (time<=timing.times[0].start+_startDelay
             || updateActiveCount()==MAX_TIMES) return false; //No valid timing for our table or list full

         memmove(&timing.times[1],&timing.times[0],sizeof(timeRec_t)*(MAX_TIMES-1));
         timing.times[0].user=timing.nextUser++; //Auto increment the user
         timing.times[0].state=updateState;
         timing.times[0].start=time;
         timing.times[0].time=0;
         if (_restoreUser) {
           timing.nextUser=_restoreUser;
           _restoreUser=0;
          }
         if (timing.nextUser>=1000) timing.nextUser=1;
   //      Serial.println(F("New Start"));
       }

     //See if there is a open record in our table for this source to close
    } else {
       addtime_retry:
       pos=MAX_TIMES-1;
       for (;pos>=0 && (!timing.times[pos].start || timing.times[pos].state>=sourceAddress);pos--){}
       if (pos>=0) {
           timing.times[pos].time=(timing.times[pos].start>time) ? 0 : time-timing.times[pos].start; //Keep in mind possible DRIFT, causing a negative
           if (updateState>=MII_STATE_FINISH) { //We are finishing
              if (timing.times[pos].start>time+_drift || timing.times[pos].time<_minBoundary) {
                //Just Skip the time it would be to small

              } else if (_maxBoundary!=0 && timing.times[pos].time>_maxBoundary) {
                 //The time is to big, make it DNF and finish it
                 timing.times[pos].state=MII_STATE_DNF;
                 goto addtime_retry;
              } else { //We have a normal finish
//                 Serial.print("Finish Time ");Serial.println(updateState);
                timing.times[pos].state=updateState;
              }
           } else { //Intermediate update
//                   Serial.println("Intermediate Time");
              timing.times[pos].state=updateState;
           }
       } else { //We did not find a start for this master to close
        _unusedAddress=sourceAddress;
        _unusedTime=time;
        // Serial.println(F("Invalid add 2"));
        return false;
       }
    }
addTime_exit:
  //Update the active count
  updateActiveCount();
  _isChanged=true;
  return true;
}

//Swap the last finish time, by closing the one after, user is taken over
bool MiiRFDevice::swapFinish(){
  if (timing.mode & MII_MODE_PARALLEL) return false; //Parallel mode not supported
  //Find the last closed time
  int lpos=0;
  for (;lpos<MAX_TIMES && timing.times[lpos].state<MII_STATE_FINISH;lpos++){}
  //Find the last active time
  int  apos=MAX_TIMES-1;
  for (;apos>=0 && (!timing.times[apos].start || timing.times[apos].state>=MII_STATE_FINISH);apos--){}
  if (apos>=0 && lpos<MAX_TIMES) {
    //Swap states
    timing.times[apos].state = timing.times[lpos].state;
    timing.times[lpos].state = 0;
    //Swap time
    timing.times[apos].time = timing.times[lpos].time-(timing.times[apos].start-timing.times[lpos].start);
    timing.times[lpos].time = 0;
    updateActiveCount();
    return sendTiming();
  }

  return false;
}

bool MiiRFDevice::noFinish(){
 if (timing.mode & MII_MODE_PARALLEL) return false; //Parallel mode not supported
  //Find the last closed time
  int lpos=0;
  for (;lpos<MAX_TIMES && timing.times[lpos].state<MII_STATE_FINISH;lpos++){}
  if (lpos<MAX_TIMES) {
    //Clear the state of finish rec
    timing.times[lpos].state = 0;
    updateActiveCount();
    uint8_t lastClient,firstClient,clientCount;
    firstLastAddress(firstClient,lastClient,clientCount);
    if (_unusedTime!=0 && lastClient==_unusedAddress) {
        addTiming(_unusedTime,_unusedAddress);
        _unusedTime=0; //Clear the unused Time
    }
    return sendTiming();
  }
  return false;
}

void  MiiRFDevice::timingChanged(){
 _isChanged=false;
}

void MiiRFDevice::modeChanged(){
  if (_timingMode & MII_MODE_PARALLEL) {
    _startDelay=MII_PARALLEL_START_DELAY;
  } else {
   _startDelay=MII_DEFAULT_START_DELAY;
  }
}

bool MiiRFDevice::setMode(uint8_t mode){
  if (!isMaster()) {
     return sendAckCmd(MII_CMD_SET_MODE,&mode,sizeof(mode));//Send mode command to master
  } else if (((timing.mode & 0xF0) !=  (mode & 0xF0))) {
      clearData();
      _timingMode=(mode & 0xF0); //Set the operation mode
      timing.mode=(mode & 0xF0) | (timing.mode & 0x0F);
      modeChanged();
      return true;
  }
  return false;
}

bool MiiRFDevice::sendTiming(){
  if (!isMaster()) return false;
  _sendTiming=millis();
  bool ret;
  if (ret=sendCmd(MII_CMD_DATA,(uint8_t*)&timing,sizeof(timing_t))) timingChanged();
  return ret;
 }

//Set the active ID
bool MiiRFDevice::setNextUserId(uint16_t id){
   if (!isMaster()) return sendAckCmd(MII_CMD_SET_USER,(uint8_t*)&id,sizeof(id));
   if (id==timing.nextUser) return false;
   //Incase of RFID changing keep the original one
   if (id>=1000 && timing.nextUser<1000) _restoreUser=timing.nextUser;
   timing.nextUser=id;
   return sendTiming();
}

bool MiiRFDevice::setSession(uint16_t session){
  if (!isMaster()) return sendAckCmd(MII_CMD_SET_SESSION,(uint8_t*)&session,sizeof(session)); //Send mode command to master
  timing.sessionId=session; //Set the operation mode
  return sendTiming();
}

bool MiiRFDevice::setSessionDate(uint32_t date){
  if (!isMaster()) return sendAckCmd(MII_CMD_SET_DATE,(uint8_t*)&date,sizeof(date)); //Send date command to master
  timing.sessionDate=date-(time()/1000); //Set the operation mode
  return sendTiming();
}

bool MiiRFDevice::setExtra(uint16_t extra){ //Set the extra information
  if (!isMaster()) return sendAckCmd(MII_CMD_SET_EXTRA,(uint8_t*)&extra,sizeof(extra));
  timing.extra=extra; //Set the extra info
  return sendTiming();
}


bool MiiRFDevice::setStartDelay(uint32_t value){
 _startDelay=value; //We are the first device so set the start delay
 return sendAckCmd(MII_CMD_SET_DELAY,(uint8_t*)&value,sizeof(value));
}

bool MiiRFDevice::setBoundary(uint32_t minvalue,uint32_t maxvalue){
 if (!isMaster()) {
   boundaryRec_t rec = {minvalue,maxvalue};
   return sendAckCmd(MII_CMD_SET_BOUNDARY,(uint8_t*)&rec,sizeof(boundaryRec_t));
 }
 if (minvalue!=0xFFFFFFFF) _minBoundary=minvalue;
 if (maxvalue!=0xFFFFFFFF) _maxBoundary=maxvalue;
 return true;
}

//Make the last open time the state. example Did Not Finish
bool MiiRFDevice::setState(stateRec_t &state){
  if (!isMaster()) return sendAckCmd(MII_CMD_SET_STATE,(uint8_t*)&state,sizeof(stateRec_t));
  for (uint8_t pos=0;pos<MAX_TIMES;pos++) {
    if (timing.times[pos].start && timing.times[pos].start==state.start) {
      timing.times[pos].state=state.state;
      updateActiveCount();
      return sendTiming();
    }
  }
  return false;
}

void MiiRFDevice::clearData(){
  MII_MODEM_CLASS::clearData();
  memset(&timing.times,0,sizeof(timeRec_t)*MAX_TIMES);
  if (timing.mode==0) timing.mode=MII_MODE_TIME; //Default when we startup we are in time mode
  timing.sessionDate=0;
  timing.nextUser=1;
  _restoreUser=0;
}

bool MiiRFDevice::changeUser(changeuser_ptr rec){
  if (!isMaster()) return sendAckCmd(MII_CMD_CHANGE_USER,(uint8_t*)rec,sizeof(changeuser_t));
  uint8_t lastClient = min(_thisAddress,MII_DEV_CLIENT);
  for (uint8_t i=0;i<_addressCount;i++) {
    if (_address[i].id>lastClient && _address[i].id<MII_DEV_CLIENT)
      lastClient = _address[i].id;
  }
  //Find the time
  uint8_t state =(rec->from==lastClient ?  MII_STATE_FINISH : rec->from);
  int pos=0;
  for (;pos<MAX_TIMES && timing.times[pos].start && timing.times[pos].state<state;pos++){}
  if (pos<MAX_TIMES && timing.times[pos].state==state && timing.times[pos].time==rec->time-timing.times[pos].start){
      if ( timing.times[pos].user!=rec->user) {
        timing.times[pos].user=rec->user;
        return sendTiming();
      }
  }
  return false;
}

bool MiiRFDevice::internalProcess(){
  bool ret = false;
  if (MII_MODEM_CLASS::internalProcess()) {
    bool master=isMaster();
    bool broadcast=_rxHeaderFrom==MII_BROADCAST_ADDRESS;
    switch (_rxHeaderId){
      case MII_CMD_CLEAR: {
       recv();clearData();sendTiming();
      }break;

      case MII_CMD_SWITCH: {
        uint32_t time;
        if (recv32_t(time) && master) {
           bool ack=addTiming(time,_rxHeaderFrom); //We will send ack
           if (sendAckCmd(MII_CMD_SWITCH_ACK,(uint8_t*)&ack,sizeof(bool),_rxHeaderFrom)) {
             if (ack) {
               //Process a unused time of master
               if (_unusedTime!=0) {
                 addTiming(_unusedTime,_unusedAddress);
                 _unusedTime=0; //Clear the unused Time
               }
               sendTiming();
             }
          }
        }
      }break;

     case MII_CMD_DATA : {
       uint8_t len=sizeof(timing_t);
       if (recv((uint8_t*)&timing,&len) && !master) {
         if (timing.mode & 0xF0 != _timingMode) {
           _timingMode=timing.mode & 0xF0;
           modeChanged();
         }
         timingChanged();
        }
      } break;

      case MII_CMD_COUNT_DOWN : { //Master has only trigger count down
         if (isMaster()) {
           recv(); //Remove the message
           //Send the count down value
           _countdown=time()+MII_COUNT_DOWN_TIME;
           //Broadcast countdown, we do it twice to make sure it is received by other devices
           sendCmd(MII_CMD_COUNT_DOWN,_countdown);
           sendCmd(MII_CMD_COUNT_DOWN,_countdown);
         } else {
           uint8_t len = sizeof(_countdown);
           recv((uint8_t*)&_countdown,&len);
         }
      } break;

      default : {
        ret = broadcast; //If we are broadcast we should return true, we have data not processed
      }
    }

    //We have split switch, here only not broadcast commands
    if (!broadcast) switch(_rxHeaderId) {
      case MII_CMD_SET_USER: {
        uint16_t id;
        if ((recv16_t(id))) setNextUserId(id);
      }break;

      case MII_CMD_SET_EXTRA : {
        uint16_t extra;
        if ((recv16_t(extra))) setExtra(extra);
      }break;

      case MII_CMD_SET_SESSION : {
        uint16_t session;
        if (recv16_t(session)) setSession(session);
      }break;

      case MII_CMD_SET_DATE : {
        uint32_t date;
        if (recv32_t(date)) setSessionDate(date);
      }break;

      case MII_CMD_SET_BOUNDARY : {
        boundaryRec_t rec;
        uint8_t len=sizeof(boundaryRec_t);
        if (recv((uint8_t*)&rec,&len))
          setBoundary(rec.minBoundary,rec.maxBoundary);
      }break;

      case MII_CMD_SET_STATE  : {
        stateRec_t state;
        uint8_t len=sizeof(stateRec_t);
        if (recv((uint8_t*)&state,&len))
         setState(state);
      }break;

      case MII_CMD_SET_MODE : {
         uint8_t mode;
         if (recv8_t(mode)) setMode(mode);
      }break;

      case MII_CMD_SWAP_FINISH : {
        uint32_t t;
        uint8_t len=sizeof(uint32_t);
        if (master && recv((uint8_t*)&t,&len)) {
           if (t!=_lastAction) swapFinish();
           _lastAction=t;
        }
      }break;
      case MII_CMD_NO_FINISH : {
        uint32_t t;
        uint8_t len=sizeof(uint32_t);
        if (master && recv((uint8_t*)&t,&len)) {
           if (t!=_lastAction) noFinish();
            _lastAction=t;
        }
      }break;

      case MII_CMD_CHANGE_USER : {
        changeuser_t rec;
        uint8_t len=sizeof(changeuser_t);
        if (recv((uint8_t*)&rec,&len)) changeUser(&rec);
      } break;


      default: {
        ret=true;
      }break; //Data is available for other to process
    }
  }
  return ret;
}


bool MiiRFDevice::available(){
 bool ret = MII_MODEM_CLASS::available();
 if (!ret) { //Internal process loop
   //Check if we should start a count down
   if (_countdown!=0 && _countdown<time()) {
     if (isMaster()) {
      #if MII_MAX_ADDRESS
       addAddress(MII_DEV_COUNT_DOWN,0);
	    #endif
	    uint8_t _from = _txHeaderFrom;
	    setHeaderFrom(MII_DEV_COUNT_DOWN);
	    sendCmd(MII_CMD_SWITCH,_countdown); //Just inform all other clients about the switch of count down
	    setHeaderFrom(_from);
      if (addTiming(_countdown,MII_DEV_COUNT_DOWN))
        sendTiming();
     }
     countdownGo();

     //Reset the counter
     _countdown=0;
   }
 }
 return ret;
}

