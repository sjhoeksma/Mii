#include <MiiEspCom.h>

#define BT_READY 100
#define MII_ESP_START "ESP-"
#define MII_ESP_CMD "ESPC"

MiiEspCom::MiiEspCom(Stream &stream) :
 serial(stream)
{}


bool MiiEspCom::begin(){
  _isConnected=true;
  send(MII_ESP_CMD_AT,0,0,false);
  if (!(available() && !_isAction && _cmd==MII_ESP_CMD_AT)) _isConnected=false;
  return _isConnected;
}

uint8_t MiiEspCom::available(uint16_t timeout){
 if (!_isConnected) return 0;
  uint32_t time = millis()+timeout;
  do {
    while (!_cmd && serial.available()){
      uint8_t r = serial.read();

      if (_pos==sizeof(MII_ESP_START)-1) { //\0 makes string 1 longer
         _cmd=r;
      } else if (r==(uint8_t)MII_ESP_START[_pos]) {
        _isAction=true;
        _pos++;
      } else if (r==(uint8_t)MII_ESP_CMD[_pos]) {
        _isAction=false;
        _pos++;
      } else {
        _pos=0;
      }
    }
    YIELD;
  } while (!_cmd && timeout && time>millis());
  return _cmd;
}

bool MiiEspCom::read(uint8_t* data,uint8_t len,uint16_t timeout){
  if (!available()) return false;
  clear();
  uint8_t* ptr=data;
  unsigned long time=millis()+timeout;
  while (len>0 && time>millis()) {
    if (serial.available()) {
      *ptr=(uint8_t)serial.read();
      ptr++;
      len--;
      if (len==0) return true;
    }
  }
  return false;
}

bool MiiEspCom::send(uint8_t cmd,const char* buf,bool isAction){
   return send(cmd,(uint8_t *)buf,strlen(buf),isAction);
}

bool MiiEspCom::send(uint8_t cmd,uint8_t* buf,uint8_t len,bool isAction){
    if (!_isConnected) return false;
    serial.print(isAction ? F(MII_ESP_START) : F(MII_ESP_CMD));
    serial.write(cmd);
    serial.write(len);
    if (len) serial.write(buf,len);
   return true;
}

void MiiEspCom::clear(){
 if (_cmd) {
    _cmd=0;
    _pos=0;
  }
}

