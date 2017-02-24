#include "OnOff.h"

//doTimed ---------------------------------------------------
typedef struct {
  OnOffFunction callback;
  uint32_t start;
  uint16_t timems;
  uint16_t sleep;
  uint8_t  loops;
  uint8_t  state;
  uint8_t  active;
} onoffEvent_t, *onoffEvent_ptr;

onoffEvent_t onoff_events[ONOFF_SIZE];
int onoff_count=0;
void processOnOff(){
  uint32_t t = millis();
  for (int i=onoff_count-1;i>=0 && onoff_count>0;i--) {

    if (onoff_events[i].start<=t && !onoff_events[i].active) {
      onoff_events[i].active=1;
      //Call the callback
      onoff_events[i].callback(onoff_events[i].state); //on
      onoff_events[i].start=t;

    } else if (t>onoff_events[i].start+onoff_events[i].timems) {

      if (onoff_events[i].active) {
        onoff_events[i].active=0;
        onoff_events[i].callback(!onoff_events[i].state); //Off
      }
      //Set the next time to start
      onoff_events[i].start=t+onoff_events[i].sleep; //We sleep
      //Check if we have a cycle count
      if (onoff_events[i].loops>1) {
        if (onoff_events[i].loops!=0xFF) onoff_events[i].loops--;
      } else {
        //remove
        onoff_count--;
        memmove(&onoff_events[i],&onoff_events[i+1],(ONOFF_SIZE-1-i) * sizeof(onoffEvent_t));
       // memset(&onoff_events[onoff_count],0,sizeof(onoffEvent_t)*(ONOFF_SIZE-onoff_count));
      }
    }
  }
}

bool doOnOff(OnOffFunction func,uint8_t onState,uint16_t timems,uint16_t sleepms,uint8_t loops,uint16_t startms){
  int i;
  for (i=0;i<onoff_count && onoff_events[i].callback!=func;i++){}
  if (i==ONOFF_SIZE) return false;
  if (i==onoff_count){ //New onoff
   onoff_count++;
   onoff_events[i].active = (timems==0); //When now time we see it as active and remove it on next run
   onoff_events[i].callback = func;
  } else { //Existing onoff
    if (onoff_events[i].active) {
      onoff_events[i].active = (onoff_events[i].state==onState);
    } else {
      onoff_events[i].active = (timems==0);
    }
  }
  onoff_events[i].start = (startms==0 ? millis(): startms);
  onoff_events[i].timems = timems;
  onoff_events[i].loops = loops;
  onoff_events[i].sleep = sleepms;
  onoff_events[i].state = onState;
  processOnOff(); //Call the processing
  return true;
}

bool delOnOff(OnOffFunction func){
  for (int i=onoff_count-1;i>=0 && onoff_count>0;i--) {
     if (onoff_events[i].callback==func) {
        //Fire disable call when active
        if (onoff_events[i].active) {
          onoff_events[i].callback(!onoff_events[i].state); //Off
        }
        //remove
        onoff_count--;
        memmove(&onoff_events[i],&onoff_events[i+1],(ONOFF_SIZE-1-i) * sizeof(onoffEvent_t));
     }
  }
}

