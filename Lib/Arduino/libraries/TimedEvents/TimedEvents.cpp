#include <TimedEvents.h>

void _TimedEvents::reset(){
 #ifdef TIMED_EVENTS_DYNAMIC
  free(_events);
  _events=NULL;
 #endif
  _count=0;
}

void _TimedEvents::add(teCallbackFunction callback,unsigned long timems,unsigned long sleepms,unsigned int loops,char group,unsigned long startms){
  uint8_t i;
  if (group) {i=_count;}
  else for (i=0;i<_count && _events[i].callback!=callback;i++){}
  if (i==_count) {
    #ifndef TIMED_EVENTS_DYNAMIC
      if (_count==TIMED_EVENTS_SIZE) return;
    #endif
   //Add a new command
    _count++;
     #ifdef TIMED_EVENTS_DYNAMIC
    _events = (timedEvent_ptr)realloc(_events,_count * sizeof(timedEvent_t));
    #endif
 }
 _events[i].active = 0;
 _events[i].timems = timems;
 _events[i].start = startms ? startms : millis();
 _events[i].loops = loops;
 _events[i].group = group;
 _events[i].sleep = sleepms;
 _events[i].callback = callback;
 process(); //Call the processing
}

void _TimedEvents::add(teCallbackFunction callback,unsigned long timems,unsigned long sleepms,unsigned int loops){
  add(callback,timems,sleepms,loops,0,0);
}

void _TimedEvents::add(teCallbackFunction callback,unsigned long timems){
  add(callback,timems,0,1,0,0);
}

void _TimedEvents::remove(teCallbackFunction callback,uint8_t all){
    for (int i=_count-1; i>=0; i--) {
        if (_events[i].callback == callback && !_events[i].active && !_events[i].loops) {
          remove(i);
          if (!all) return;
        }
    }
}

void _TimedEvents::remove(uint8_t pos){
   if (pos<_count){
    memcpy(&_events[pos],&_events[pos+1],(_count-1-pos) * sizeof(timedEvent_t));
    _count--;
    #ifdef TIMED_EVENTS_DYNAMIC
      _events=(timedEvent_ptr) realloc(_events,_count * sizeof(timedEvent_t));
    #endif
  }
}

void _TimedEvents::process(){
  unsigned long t = millis();
  for (int i=_count-1;i>=0 && _count>0;i--) {
    if (_events[i].start<=t && !_events[i].active) {
      _events[i].active=1;
      //Check if this event is part of a group
      if (_events[i].group) {
        //Check if there is a item of same group high in stack whe should not turn on
        for (int j=i+1;j<_count;j++) {
          if (_events[i].group==_events[j].group && _events[j].active) {
            _events[i].active=0;
            break; //Exit for
          }
        }
        //Check if we should turn of lower items in the stack of same group
        if (_events[i].active) for (int j=i-1;j>=0;j--) {
          if (_events[i].group==_events[j].group && _events[j].active) {
             _events[j].callback(0); //Turn the same in group off
             _events[j].active=0;
          }
        }
      }
     //Call the callback
      if (_events[i].active) _events[i].callback(1); //on
    }

    if (t>_events[i].start+_events[i].timems) {
      if (_events[i].active) {
        _events[i].callback(0); //Off
        _events[i].active=0;
      }
      //Set the next time to start
      _events[i].start=t+_events[i].sleep; //We sleep

      //Check if we have a cycle count
      if (_events[i].loops>1) {
        if (_events[i].loops!=0xFFFF) _events[i].loops--;
      } else {
         remove(i);
      }
    }
  }
}

//The real class;
_TimedEvents TimedEvents;