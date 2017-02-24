#ifndef MiiESPCom_h
#define MiiESPCom_h 1

#include <Mii.h>

//----------------------------------------------------------------------
//Constants the BlueTooth
//----------------------------------------------------------------------
#ifndef MII_ESP_BAUD
#define MII_ESP_BAUD 38400
#endif

#define MII_ESP_CMD_AT 1
#define MII_ESP_CMD_OK 2
#define MII_ESP_CMD_FAIL 3

class MiiEspCom {
public:
  MiiEspCom(Stream &stream=Serial);
  bool begin();
  bool isConnected(){return _isConnected;}
  uint8_t available(uint16_t timeout=0);
  bool read(uint8_t* data,uint8_t len,uint16_t timeout=0);
  bool send(uint8_t cmd,uint8_t* buf,uint8_t len,bool action=true);
  bool send(uint8_t cmd,const char* buf,bool action=true);
  void clear();
  uint8_t getCmd(){return _cmd;}
  bool isAction(){return _isAction;}

protected:
  bool _isConnected;
  Stream &serial;
  uint8_t _cmd;
  uint8_t _pos;
  bool _isAction;

};

#endif