#include <MiiRFClient.h>


 ///Init the based independent of RF device used
MiiRFClient::MiiRFClient(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
  : MII_MODEM_CLASS(selectPin,intPin,sdnPin) {
   setDrift(1000); //By default we allow a client to drift 1 second before forcing sync again
   setTimeSync(false);
}

//Clear the data list
void MiiRFClient::clearData(){
 MII_MODEM_CLASS::clearData();
 memset(&timing.times,0,sizeof(timeRec_t)*MAX_TIMES);
 memset(&finishTime[0],0,sizeof(timeRec_t)*MII_MAX_FINISH_TIMES);
}

//Update the data we have received
void MiiRFClient::updateData(){
 int pos;
  if ((timing.mode & MII_MODE_TIME) || (timing.mode & MII_MODE_PARALLEL)) {
    activeCount=(timing.mode & 0x0F);
    nextUser=timing.nextUser;
    if (timing.times[0].start<ntiming.times[0].start) { //New start time, clear the data
      clearData();
    } else for (int i=0;i<MAX_TIMES;i++){
      if (timing.times[i].start>ntiming.times[0].start){ //We have a new data
        changed(&timing.times[i],NULL);
        start(timing.times[i]);
      } else {
         for (int j=0;j<MAX_TIMES;j++){
           //We see same time as start and user equal
          if (timing.times[i].start==ntiming.times[j].start &&
	         timing.times[i].user!=ntiming.times[j].user){ //user  has changed
            changed(&timing.times[i],&ntiming.times[j]);
            for (int f=0;f<MII_MAX_FINISH_TIMES;f++) {
		          if (finishTime[f].start==timing.times[i].start) {
		           finishTime[f].user=timing.times[i].user; //Rename user of finished
		           break;
		          }
	          }
          }
         //We see same time as start and user equal
         if (timing.times[i].start==ntiming.times[j].start &&
	         (timing.times[i].time!=ntiming.times[j].time ||
	         timing.times[i].state!=ntiming.times[j].state)
	        ){ //Time or state has changed
	          changed(&timing.times[i],&ntiming.times[j]);
            if (timing.times[i].time>0 && timing.times[i].state<MII_STATE_FINISH) {
              intermediate(timing.times[i]);
            }
            //Remove item if it was in finish list
            for (int f=0;f<MII_MAX_FINISH_TIMES;f++) {
              if (timing.times[i].start==finishTime[f].start) { //Add item to finish list
	                memmove(&finishTime[f],&finishTime[f+1],sizeof(timeRec_t)*(MII_MAX_FINISH_TIMES-1-f));
	                finishTime[MII_MAX_FINISH_TIMES-1].start=0; //Ensure last rec is empty
                  break; //Exit For loop
	            }
            }
          }
       }
      }
    }

    //Now we can make the ntiming equal the timing
    ntiming=timing;
    int pos=MAX_TIMES-1;
    int lpos=-1;
    //Go through the list with active times, see if there is a one to close one add it to top of list
    for (;pos>=0;pos--){
      if (timing.times[pos].start==0) { //Just Skip empty time
      } else if  (timing.times[pos].state>=MII_STATE_FINISH) { //Check if item is not in finish list,
        int f=0;
        for (;f<=MAX_TIMES && timing.times[pos].start!=finishTime[f].start;f++) {}
        if (f>MAX_TIMES) { //Not Found add it
          memmove(&finishTime[1],&finishTime[0],sizeof(timeRec_t)*(MII_MAX_FINISH_TIMES-1));
          finishTime[0]=timing.times[pos];
          finish(finishTime[0]);
        }
	    } else if (lpos==-1) { //Active Item
	      lpos=pos; //Last Active Pos
	    }
    }

    if (lpos>=0) {
      activeTime=timing.times[lpos];
    } else {
      activeTime.start=0;
      activeTime.user=nextUser;
      activeTime.state=0;
    }
  }
}

//Overwrite default available to process the data elements
bool MiiRFClient::internalProcess(){
  bool ret = false;
  if (MII_MODEM_CLASS::internalProcess()) {
    switch (_rxHeaderId){
      case MII_CMD_DATA: {
        uint8_t len = sizeof(timing_t);
        if ((recv((uint8_t*)&timing,&len))) {
         heartbeat();
         updateData();
        }
      } break;
      case MII_CMD_COUNT_DOWN:
        if  (_rxHeaderFrom==_masterAddress) {
          recv32_t(_countdown);
          break;
        }//Fall to default
      default:
        ret=true;break;
      }
    }
    return ret;
 }


bool MiiRFClient::setSessionDate(uint32_t sessionDate){ //Set the sessionDate by sending it to master
  return sendAckCmd(MII_CMD_SET_DATE,(uint8_t*)&sessionDate,sizeof(sessionDate));
}

bool MiiRFClient::setNextUserId(uint16_t  userId){ //Set the next user id by sending it to master
 if (sendAckCmd(MII_CMD_SET_USER,(uint8_t*)&userId,sizeof(userId))){
   nextUser=userId;
   return true;
  }
  return false;
}

bool MiiRFClient::setExtra(uint16_t extra){ //Set the extra information
return sendAckCmd(MII_CMD_SET_EXTRA,(uint8_t*)&extra,sizeof(extra));
}

bool MiiRFClient::setState(uint8_t state){ //Set the state of active
  stateRec_t newState = {activeTime.start,state};
  return sendAckCmd(MII_CMD_SET_STATE,(uint8_t*)&newState,sizeof(stateRec_t));
}

bool MiiRFClient::changeUser(changeuser_ptr rec){
  return sendAckCmd(MII_CMD_CHANGE_USER,(uint8_t*)rec,sizeof(changeuser_t));
}

bool MiiRFClient::setDNF(){
  return setState(MII_STATE_DNF);
}

bool MiiRFClient::setStartDelay(uint32_t value){
 return sendAckCmd(MII_CMD_SET_DELAY,(uint8_t*)&value,sizeof(value));
}

bool MiiRFClient::setBoundary(uint32_t minvalue,uint32_t maxvalue){
   boundaryRec_t rec = {minvalue,maxvalue};
   return sendAckCmd(MII_CMD_SET_BOUNDARY,(uint8_t*)&rec,sizeof(boundaryRec_t));
}

bool MiiRFClient::setMode(uint8_t mode){ //Set the operation mode
   return sendAckCmd(MII_CMD_SET_MODE,(uint8_t*)&mode,sizeof(mode));
}

bool MiiRFClient::swapFinish(){ //Swap the last finish
   return sendAckCmd(MII_CMD_SWAP_FINISH,time());
}

bool MiiRFClient::noFinish(){ //Swap the last finish
   return sendAckCmd(MII_CMD_NO_FINISH,time());
}



