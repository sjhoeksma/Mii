#ifndef _ONOFF_H_
#define _ONOFF_H_ 1
#include <Arduino.h>

//OnOff is function to controll pin function is specific taks ,Buzzer and Led, LCD backlight
#define ONOFF_SIZE 3
// Format of command callback functions of onoff functions
typedef void(*OnOffFunction)(uint8_t);

//Added a OnOffFunction and there specific times
extern bool doOnOff(OnOffFunction func,uint8_t onState,uint16_t timems,uint16_t sleepms=0,uint8_t loops=0,uint16_t startms=0);
extern void processOnOff();
extern bool delOnOff(OnOffFunction func);
#endif

