#ifndef TimedEvents_h
#define TimedEvents_h

#include <Arduino.h>


// Uncomment the definition of TIMED_EVENTS_DYNAMIC if you want to use
// dynamic memory allocation
//#define TIMED_EVENTS_DYNAMIC

#ifndef TIMED_EVENTS_DYNAMIC
//The maximal number of parallel TimedEvents
#define TIMED_EVENTS_SIZE 10
#endif

// Format of command callback functions
typedef void(*teCallbackFunction)(uint8_t);

typedef struct {
  teCallbackFunction callback;
  unsigned long start;
  unsigned long timems;
  unsigned long sleep;
  unsigned int  loops;
  char     group;
  uint8_t  active;
} timedEvent_t, *timedEvent_ptr;

class _TimedEvents {
  public:
    ~_TimedEvents(){reset();}
    void add(teCallbackFunction callback,unsigned long timems,unsigned long sleepms,unsigned int loops,char groups,unsigned long startms=0);
    void add(teCallbackFunction callback,unsigned long timems,unsigned long sleepms,unsigned int loops);
    void add(teCallbackFunction callback,unsigned long timems);
    void process();
    void remove(teCallbackFunction callback,uint8_t all=0);
    void remove(uint8_t pos);
    void reset();

  protected:
    uint8_t _count;
  #ifdef TIMED_EVENTS_DYNAMIC
    timedEvent_ptr _events;
  #else
    timedEvent_t   _events[TIMED_EVENTS_SIZE];
  #endif
};

extern _TimedEvents TimedEvents;

#endif