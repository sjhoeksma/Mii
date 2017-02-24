#include "TWIChannel.h"
#include <Wire.h>

// Initialize Class Variables //////////////////////////////////////////////////
char TwiChannel::state=0;
byte TwiChannel::pollSize;
byte TwiChannel::pollId;
uint8_t TwiChannel::_commandCount=0;
#ifdef TWI_CHANNEL_DYNAMIC
twiCommand_ptr TwiChannel::_commands=NULL;
#else
 twiCommand_t  TwiChannel::_commands[TWI_CHANNEL_MAX];
#endif
uint8_t TwiChannel::_isRequest=0;


TwiChannel::~TwiChannel(){
 #ifdef TWI_CHANNEL_DYNAMIC
 free(_commands);
 #endif
 }

void TwiChannel::process(){
  if (lastScan<millis()) {
    lastScan=millis()+SCANINTERVAL; //Next scan
    scan();
  }
}

boolean TwiChannel::callback(byte eventid,byte *data,byte len) {
  boolean ret=false;
  for (uint8_t i=0; i<_commandCount; i++) {
    if (_commands[i].commandCode == eventid && _commands[i].callback!=NULL) {
      ret|=_commands[i].callback(eventid,data,len);
    }
  }
  return ret;
}

void TwiChannel::receiveService(int howMany){
   _isRequest=0;
   byte size;
   if (Wire.available()) {
     state = Wire.read();
     switch (state) {
       case POLL_FLAG : {
        pollId=Wire.available() ? Wire.read() : 0xFF;
        pollSize=Wire.available() ? Wire.read() : 0;
       }
       break;

       case EVENT_FLAG : {
        byte id;id=Wire.available() ? Wire.read() : 0xff;
        size=Wire.available() ? Wire.read() : 0;
        byte data[size];
        for (byte i=0;i<size && Wire.available();i++) {data[i]=Wire.read();}
        callback(id,data,size);
        }
        break;

      default: {
       size=howMany-1;
       byte data[size];
       for (byte i=0;i<size && Wire.available();i++) {data[i]=Wire.read();}
       callback(state,data,size);
       break;
      }
     }
   }
}

//Becarefull repleys must be given in one go.
void TwiChannel::requestService(void) {
    _isRequest=1;
    switch (state) {
      case START_FLAG : {
        byte d[]={CONNECT_FLAG,_commandCount};
        Wire.write(d,sizeof(d));
        state=CONNECT_FLAG; //Next request will be connect
        }break;
      case CONNECT_FLAG: {
        byte d[_commandCount+1];
        d[0]=REGISTER_FLAG;
        for (int i=0;i<_commandCount;i++) d[i+1]=_commands[i].commandCode;
        Wire.write(d,_commandCount+1);
        state=EVENT_FLAG; //Next request should be event request
        }break;
       case POLL_FLAG : {
        byte data[pollSize+1];
        data[0]=callback(pollId,&data[1],pollSize) ? DATA_FLAG : IDLE_FLAG;
        Wire.write(data,pollSize+1);
       } break;
      default:
         break;
    }
 }

void TwiChannel::registerEvent(byte eventid, twiCallbackFunction func){
  uint8_t i;
  for (i=0; i<_commandCount; i++) {
     if (_commands[i].commandCode == eventid) {
        _commands[i].callback = func;
       return;
      }
  }
 // Not Found add a new command
 #ifndef TWI_CHANNEL_DYNAMIC
   if (_commandCount+1==TWI_CHANNEL_SIZE) return;
 #endif
  _commandCount++;
 #ifdef TWI_CHANNEL_DYNAMIC
 _commands = (twiCommand_ptr)realloc(_commands,_commandCount * sizeof(twiCommand_t));
 #endif
 _commands[i].commandCode=eventid;
 _commands[i].callback = func;
}

void TwiChannel::unregisterEvent(byte eventid){
    for (uint8_t i=0; i<_commandCount; i++) {
        if (_commands[i].commandCode == eventid) {
            _commandCount--;
            memcpy(&_commands[i],&_commands[i+1],(_commandCount-i) * sizeof(twiCommand_t));
          #ifdef TWI_CHANNEL_DYNAMIC
            _commands=(twiCommand_ptr) realloc(_commands,_commandCount * sizeof(twiCommand_t));
          #endif
            return;
        }
    }
}

//Master begin
void TwiChannel::begin(){
 Wire.begin();
 scan();
}


//Client begin
void TwiChannel::begin(byte id){
  Wire.begin(id);
  Wire.onRequest(requestService);
  Wire.onReceive(receiveService);
}


byte TwiChannel::read(byte eventid,byte *dataptr,byte datasize){
  byte *p = data_ptr;
  while (data_size!=0 && (data_ptr+data_size) != p) {
    byte addr=*p;p++;
    byte len=*p;p++;
    while (len>0) {
      len--;
      if (*p==eventid) {
       Wire.beginTransmission(addr); // transmit to device
       Wire.write(POLL_FLAG);
       Wire.write(eventid);
       Wire.write(datasize);
       Wire.endTransmission();
       Wire.requestFrom((uint8_t) addr,(uint8_t) (1+datasize));
       char c;
       if (Wire.available() && (c=Wire.read())==DATA_FLAG) {
         while (Wire.available() && datasize) { //Read the data loop and fill array
           *dataptr=Wire.read();
           dataptr++;
           datasize--;
         }
         return 1; //We have valid read
       }
      }
      p++;
    }
  }
  return 0;
}

void TwiChannel::write(byte eventid,byte *dataptr,byte datasize){
  byte *p = data_ptr;
  while (data_size!=0 && (data_ptr+data_size) != p) {
    byte addr=*p;p++;
    byte len=*p;p++;
    while (len>0) {
      len--;
      if (*p==eventid) {
       Wire.beginTransmission(addr); // transmit to device
       Wire.write(EVENT_FLAG);
       Wire.write(eventid);
       Wire.write(datasize);
       Wire.write(dataptr,datasize);
       Wire.endTransmission();
      }
      p++;
    }
  }
}


void TwiChannel::scan() {
  for( uint8_t addr = TWI_ID_MIN; addr <= TWI_ID_MAX; addr++) {
    Wire.beginTransmission (addr);
    if (Wire.endTransmission () != 0) continue;
    //Remove the data of addr
    uint8_t pos = 0;
    while (pos<data_size) {
      byte s = *(data_ptr+pos+1);s+=2; //The datablock size (addr+count+[events])
      if (*(data_ptr+pos)==addr) {
        data_size-=s;
        memcpy((byte *)(data_ptr+pos),(byte *)(data_ptr+pos+s),data_size-pos);  //Copy data behind me
     #ifdef TWI_CHANNEL_DYNAMIC
        data_ptr = (byte *) realloc( data_ptr, data_size); //Re allocated the data block
     #endif
      } else pos+=s;
    }
    //Check if we get a response from the slave
    Wire.beginTransmission(addr); // transmit to device
    Wire.write(START_FLAG);    // sends the start trigger
    Wire.endTransmission();    // stop transmitting
    Wire.requestFrom((uint8_t) addr,(uint8_t) 2);    // request 2 bytes from slave device
    if (Wire.available())  {  // slave may send less than requested
       char c = Wire.read();    // receive a byte as character
       if (c==CONNECT_FLAG) {   // We want to connect
          byte count=Wire.available() ? Wire.read() : 0; //Read the number of Events
        #ifndef TWI_CHANNEL_DYNAMIC
          if (data_size + count + 2 >= TWI_CHANNEL_SIZE) return;
        #endif
          byte action;
          data_size+=2; //Increase data size with addr+count
      #ifdef TWI_CHANNEL_DYNAMIC
          data_ptr = (byte *) realloc( data_ptr, data_size); //Re allocated the data block
      #endif
          byte *p = (data_ptr+data_size-2);
          *p=addr;p++;//set addr
          *p=count;   //set count
          Wire.requestFrom((uint8_t) addr,(uint8_t) (1+count));    // request 1 bytes from slave device #2
          if (Wire.available()) {
            c=Wire.available() ? Wire.read() : 0;
            while (count && c==REGISTER_FLAG ) { //We want to register for a event
               count--;
               action = Wire.available() ? Wire.read() : 0;
               data_size++;
            #ifdef TWI_CHANNEL_DYNAMIC
               data_ptr = (byte *) realloc( data_ptr, data_size);
            #endif
               *(data_ptr+data_size-1)=action;
             }
          }
       }
    }
  }
}

TwiChannel TWIChannel;
