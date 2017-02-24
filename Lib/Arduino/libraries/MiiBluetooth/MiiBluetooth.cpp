#include <MiiBluetooth.h>

MiiBluetooth::MiiBluetooth(){}

void MiiBluetooth::begin(Stream* serial){
 _serial=serial;
 checkInstalled();
}

void MiiBluetooth::begin(uint32_t baud){
 Serial.begin(baud);
 _serial=&Serial;
 checkInstalled();
}

//Config
void MiiBluetooth::begin(uint32_t id,uint8_t address,uint32_t baud,uint32_t initbaud){
  _serial=&Serial;
  Serial.begin(initbaud); //Default baud rate when modem is delivered
  if (checkInstalled()){
    Serial.print("AT+BAUD");_serial->print(
     baud==9600  ? 4 :
     baud==19200 ? 5 :
     baud==38400 ? 6 :
     baud==57600 ? 7 : 6) ; //We default to 38400
     delay(MII_BLUETOOTH_CMD_WAIT); //At command needs some time
    Serial.begin(MII_BLUETOOTH_BAUD); //New baud rate set by use
    Serial.print( MII_BLUETOOTH_NAME);
    Serial.print('-');
    Serial.print(id%10000);
    Serial.print('-');
    if (address<100) Serial.print('0');
    if (address<10) Serial.print('0');
    Serial.print(address);
    delay(MII_BLUETOOTH_CMD_WAIT); //At command needs some time to configure name
    Serial.begin(baud);
  }
}

//Clear the buffer and reset
void MiiBluetooth::clearBuf(){
 while (_serial && _serial->available()) _serial->read();
 clear();
}

bool MiiBluetooth::checkInstalled(){
  isInstalled=true;
  /*
  clearBuf();
  isInstalled=false;
  if (_serial) {
     _serial->print("AT");
     delay(MII_BLUETOOTH_CMD_WAIT);
     int cmd=0;
     while (_serial->available() && (cmd=_serial->read())!='O') {}
     if (cmd=='O' && _serial->read()=='K') isInstalled=true;
     clearBuf();
  }
  */
  return isInstalled;
}

uint8_t MiiBluetooth::available(uint16_t timeout){
 if (!isInstalled) return 0;
  uint32_t time = millis()+timeout;
blueAvailalbeRead:
  while (!_cmd && _serial->available()){
    uint8_t r = _serial->read();

    if (_pos==0) _pos=2; //We Skip always the first 2 - chars
    if (_pos==sizeof(MII_BLUETOOTH_START)-1) { //\0 makes string 1 longer the expectec
       _cmd=r;
    } else if (r==(uint8_t)MII_BLUETOOTH_START[_pos]) {
      _pos++;
    } else {
      _pos=0;
    }
  }
  YIELD;
  if (!_cmd && timeout && time>millis()) goto blueAvailalbeRead;
  return _cmd;
}

void MiiBluetooth::clear(){
 if (_cmd) {
    _cmd=0;
    _pos=0;
  }
}

bool MiiBluetooth::read(uint8_t* data,uint8_t len,uint16_t timeout){
  if (!available()) return false;
  clear();
  uint8_t* ptr=data;
  unsigned long time=millis()+(timeout ? timeout : 40);
  while (len>0 && (time>millis() || _serial->available())) {
    if (_serial->available()) {
      *ptr=(uint8_t)_serial->read();
      ptr++;
      len--;
      if (len==0) return true;
    }
  }
  return false;
}

bool MiiBluetooth::send(uint32_t time,uint8_t _cmd,uint8_t* buf,uint8_t len){
    if (!isInstalled) return false;
    _serial->print(F(MII_BLUETOOTH_START));
    _serial->write(_cmd);
    _serial->write((uint8_t*)&time,sizeof(time));
    _serial->write(len);
    if (len && len!=0xff) {
       _serial->write(buf,len);
    }
    #if MII_BLUETOOTH_FLUSH
      if (len!=0xff){
       _serial->flush();
      }
    #endif
   return true;
}

void MiiBluetooth::log(const byte level,const String &str,const String &str2){
 log(millis(),level,str,str2);
}

//Logging -------------------------------------------------------------
void MiiBluetooth::log(uint32_t time,const byte level,const String &str,const String &str2){
  if (!isInstalled) return;
  send(time,MII_CMD_LOG,NULL,0xff);
  _serial->print(time);
  _serial->print(':');
  switch (level) {
    case 0: break;
    case 1: _serial->print(F("FAIL: "));break;
    case 2: _serial->print(F("WARN: "));break;
    case 3: _serial->print(F("INFO: "));break;
    default: _serial->print(F("DEBUG: "));
  }
  _serial->print(str);
  if (str2.length()!=0) _serial->print(' ');
  _serial->println(str2);
  #if MII_BLUETOOTH_FLUSH
   _serial->flush();
  #endif

}

MiiBluetooth Bluetooth;
