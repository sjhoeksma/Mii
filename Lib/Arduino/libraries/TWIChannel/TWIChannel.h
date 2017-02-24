#ifndef _TwiChannel_h
#define _TwiChannel_h

#include <Arduino.h>

#define START_FLAG 'S'
#define CONNECT_FLAG 'C'
#define REGISTER_FLAG 'R'
#define EVENT_FLAG 'E'
#define POLL_FLAG 'P'
#define DATA_FLAG 'D'
#define IDLE_FLAG 'I'

// Uncomment the definition of TIMED_EVENTS_DYNAMIC if you want to use
// dynamic memory allocation
//#define TWI_CHANNEL_DYNAMIC

#ifndef TWI_CHANNEL_DYNAMIC
#define TWI_CHANNEL_MAX 5
#define TWI_CHANNEL_SIZE 25
#endif

//Define the TWI range ID range used for your own devices,
//default is 10-30 giving 20 different type of devices to connect
#define TWI_ID_MIN   10
#define TWI_ID_MAX   30

//Default scan interval is every 2.855 second
#define SCANINTERVAL 2855

// Format of command callback functions
typedef boolean(*twiCallbackFunction)(uint8_t,uint8_t *,uint8_t);

// Structure to store command code / function pairs
typedef struct {
    char commandCode;
    twiCallbackFunction callback;
} twiCommand_t, *twiCommand_ptr;

//Aparte fucties for onWrite en onRead
class TwiChannel {

public:
 TwiChannel(){}
 ~TwiChannel();
 void begin();
 void begin(byte id);
 void write(byte eventid,byte *dataptr,byte datasize);
 void write(byte eventid,byte data) {write(eventid,&data,sizeof(data));}
 void write(byte eventid,int data) {write(eventid,(byte *)&data,(byte)sizeof(data));}
 void write(byte eventid,long data) {write(eventid,(byte *)&data,(byte)sizeof(data));}
 void write(byte eventid,const char * data){write(eventid,(byte *)data,(byte)strlen(data));}
 byte read(byte eventid,byte *dataptr,byte datasize);
 void scan();
 void process();
 void registerEvent(byte eventid, twiCallbackFunction func);
 void unregisterEvent(byte eventid);
 static uint8_t isRequest(){return _isRequest;}

protected:
 static void requestService(void);
 static void receiveService(int howMany);
 static boolean callback(byte eventid,byte *data,byte len);
 static char state;
 static byte pollSize;
 static byte pollId;
 unsigned long lastScan;
 static uint8_t _commandCount;
 uint8_t data_size;

#ifdef TWI_CHANNEL_DYNAMIC
 static twiCommand_ptr _commands;
 byte *data_ptr;
#else
 static twiCommand_t _commands[TWI_CHANNEL_MAX];
 byte data_ptr[TWI_CHANNEL_SIZE];
#endif
 static uint8_t _isRequest;
};

extern TwiChannel TWIChannel;

#endif


