#ifndef _SkiData_h
#define _SkiData_h
#include <Arduino.h>

//The max item in the active list
#define SKI_MAX_DATA 5

//The value used for a DNF
#define DNF_LONG 4294967295UL

//Station_Id
#define START_DEVICE 'S'
#define INTERMEDIATE_DEVICE 'I'
#define FINISH_DEVICE 'F'
#define CLIENT_DEVICE 'C'

//Commands
#define DNF 'D'
#define HEARTBEAT     'h'
#define ACTIVEID      'a'
#define SETCONFIG     'c'
#define RESET         'r'
#define SWITCH        's'
#define CONFIG_SWITCH 'e'
#define SYSTEM_ID     'i'
#define FIRMWARE      'f'
#define NO_SYNC       'n'


//This config structure is read and stored in EEPROM
typedef struct {
    unsigned char deviceType;            //The device type used
    unsigned int  sensorTimeout;         //The min time between two switches
    unsigned int  heartbeatInterval;     //The time between start hartbeats
    unsigned int  maxActive;             //The maximum of active records
    unsigned int  breakTime;             //Time in ms for a sensor break to be accepted
    boolean       repeater;              //Should the we relay the message to other devices
    unsigned long systemId;
} config_t, *config_ptr;

/*
//The data structure used in communication
//16 bit data blocks (14 for icsc) -->18  2x16-14=18
typedef struct {
  uint32_t did;  //Data Id 4
  uint32_t sid;  //Switch Id 8
  uint16_t uid;  //User Id 10
  char  payload[8]; //Free User data 18
} transmit_t, *transmit_ptr;
*/

//Internal structure to store the times 4+2+4+4+4+4+2=26+8=34
typedef struct {
  uint32_t sessiongroup; //Group is only low byte 0x000000ff other is session
  uint16_t id;
  uint32_t start;
  uint32_t intermediate;
  uint32_t finish;
} timing_t, *timing_ptr;

typedef struct {
  timing_t timing;
  uint16_t nextId;
} broadcast_t, *broadcast_ptr;

class SkiData {
  public:
    void reset();
    byte size();
    boolean add(timing_ptr rec,boolean update=true);             //Add a new data time record
    boolean remove(timing_ptr rec,boolean update=false);         //Remove a data record
    boolean remove(byte pos=0);                                  //Remove record by position, defaults to start
    boolean read(byte pos,timing_ptr rec);                       //Read a record at given position
    int find(uint32_t start,boolean deleted=false);             //Find the position of the start, deleted true then also deleted will be searched
    boolean sync(char deviceType,timing_ptr data,char deviceTypeIn=0); //Create a synchronize record for the list based on the deviceType, when deviceTypeIn e is set sync will return if it made changes
    int finishPos(int pos);                                      //Get the postion for finish record;
    byte listEnd();            //The end of the list
    int  realPos(int pos);    //Return the real pos
    void setSyncMillis(long diffTime);
    unsigned long getSyncMillis(unsigned long Time,boolean sub=true);
    boolean noSync; //When set whe will not do any internal sync data is passed to client


 protected:
   byte          _listStart;
   byte          _listSize;
   timing_t      _list[SKI_MAX_DATA];
   long _timeDiff;                  //The diffence used for communication protocol corrections


};

#endif
