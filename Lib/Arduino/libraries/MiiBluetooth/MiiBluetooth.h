#ifndef MiiBluetooth_h
#define MiiBluetooth_h 1

#include <Mii.h>
//----------------------------------------------------------------------
//Constants the BlueTooth
//----------------------------------------------------------------------
#define MII_BLUETOOTH_FLUSH 1
#define MII_BLUETOOTH_NAME "AT+NAMEMii"
#define MII_BLUETOOTH_START "---MII"

#define MII_LOG_LEVEL_OFF   0
#define MII_LOG_LEVEL_NONE  1
#define MII_LOG_LEVEL_FAIL  2
#define MII_LOG_LEVEL_WARN  3
#define MII_LOG_LEVEL_INFO  4
#define MII_LOG_LEVEL_DEBUG 5

//Define the Bluetooth constant if not defined, to be default of
#ifndef MII_BLUETOOTH
 #define MII_BLUETOOTH MII_LOG_LEVEL_NONE
#endif

//----------------------------------------------------------------------
//Constants the BlueTooth
//----------------------------------------------------------------------
#ifndef MII_BLUETOOTH_BAUD
#define MII_BLUETOOTH_BAUD 38400
#endif
#define MII_BLUETOOTH_CMD_WAIT 750

class MiiBluetooth {
public:
  MiiBluetooth();
  void begin(Stream* serial);
  void begin(uint32_t baud=MII_BLUETOOTH_BAUD);
  void begin(uint32_t id,uint8_t address,uint32_t baud=MII_BLUETOOTH_BAUD,uint32_t initbaud=9600);
  uint8_t available(uint16_t timeout=0);
  bool read(uint8_t* data,uint8_t len,uint16_t timeout=0);
  bool send(uint32_t time,uint8_t cmd,uint8_t* buf,uint8_t len);
  void log(uint32_t time,const byte level,const String &str,const String &str2="");
  void log(const byte level,const String &str,const String &str2="");
  void clear();
  //Clear the buffer and reset
  void clearBuf();

protected:
  Stream* _serial;
  uint8_t _cmd;
  uint8_t _pos;
  bool    isInstalled;
  bool    checkInstalled();
};

//-----------------------------------------------------------------------
// The logging marco's, we can only log when Bluetooth is enabled
// value >1 defines the log level
//-----------------------------------------------------------------------
#if MII_BLUETOOTH >MII_LOG_LEVEL_OFF
extern MiiBluetooth Bluetooth;

#if MII_BLUETOOTH == MII_LOG_LEVEL_FAIL
  #define LOG_DEBUG(str)
  #define LOG_INFO(str)
  #define LOG_WARN(str)
  #define LOG_FAIL(str) Bluetooth.log(1,str);
  #define LOG_DEBUG_T(time,str)
  #define LOG_INFO_T(time,str)
  #define LOG_WARN_T(time,str)
  #define LOG_FAIL_T(time,str) Bluetooth.log(time,1,str);
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2)
  #define LOG_WARN_T2(time,str,str2)
  #define LOG_FAIL_T2(time,str,str2) Bluetooth.log(time,1,str,str2);
#elif MII_BLUETOOTH == MII_LOG_LEVEL_WARN
  #define LOG_DEBUG(str)
  #define LOG_INFO(str)
  #define LOG_WARN(str) Bluetooth.log(2,str);
  #define LOG_FAIL(str) Bluetooth.log(1,str);
  #define LOG_DEBUG_T(time,str)
  #define LOG_INFO_T(time,str)
  #define LOG_WARN_T(time,str) Bluetooth.log(time,2,str);
  #define LOG_FAIL_T(time,str) Bluetooth.log(time,1,str);
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2)
  #define LOG_WARN_T2(time,str,str2) Bluetooth.log(time,2,str,str2);
  #define LOG_FAIL_T2(time,str,str2) Bluetooth.log(time,1,str,str2);
#elif MII_BLUETOOTH == MII_LOG_LEVEL_INFO
  #define LOG_DEBUG(str)
  #define LOG_INFO(str) Bluetooth.log(3,str);
  #define LOG_WARN(str) Bluetooth.log(2,str);
  #define LOG_FAIL(str) Bluetooth.log(1,str);
  #define LOG_DEBUG(time,str)
  #define LOG_INFO_T(time,str) Bluetooth.log(time,3,str);
  #define LOG_WARN_T(time,str) Bluetooth.log(time,2,str);
  #define LOG_FAIL_T(time,str) Bluetooth.log(time,1,str);
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2) Bluetooth.log(time,3,str,str2);
  #define LOG_WARN_T2(time,str,str2) Bluetooth.log(time,2,str,str2);
  #define LOG_FAIL_T2(time,str,str2) Bluetooth.log(time,1,str,str2);
 #elif MII_BLUETOOTH >= MII_LOG_LEVEL_DEBUG
  #define LOG_DEBUG(str) Bluetooth.log(4,str);
  #define LOG_INFO(str) Bluetooth.log(3,str);
  #define LOG_WARN(str) Bluetooth.log(2,str);
  #define LOG_FAIL(str) Bluetooth.log(1,str);
  #define LOG_DEBUG_T(time,str) Bluetooth.log(time,4,str);
  #define LOG_INFO_T(time,str) Bluetooth.log(time,3,str);
  #define LOG_WARN_T(time,str) Bluetooth.log(time,2,str);
  #define LOG_FAIL_T(time,str) Bluetooth.log(time,1,str);
  #define LOG_DEBUG_T2(time,str,str2) Bluetooth.log(time,4,str,str2);
  #define LOG_INFO_T2(time,str,str2) Bluetooth.log(time,3,str,str2);
  #define LOG_WARN_T2(time,str,str2) Bluetooth.log(time,2,str,str2);
  #define LOG_FAIL_T2(time,str,str2) Bluetooth.log(time,1,str,str2);
#else
  #define LOG_DEBUG(str)
  #define LOG_INFO(str)
  #define LOG_WARN(str)
  #define LOG_FAIL(str)
  #define LOG_DEBUG_T(time,str)
  #define LOG_INFO_T(time,str)
  #define LOG_WARN_T(time,str)
  #define LOG_FAIL_T(time,str)
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2)
  #define LOG_WARN_T2(time,str,str2)
  #define LOG_FAIL_T2(time,str,str2)
#endif
#else
  #define LOG_DEBUG(str)
  #define LOG_INFO(str)
  #define LOG_WARN(str)
  #define LOG_FAIL(str)
  #define LOG_DEBUG_T(time,str)
  #define LOG_INFO_T(time,str)
  #define LOG_WARN_T(time,str)
  #define LOG_FAIL_T(time,str)
  #define LOG_DEBUG_T2(time,str,str2)
  #define LOG_INFO_T2(time,str,str2)
  #define LOG_WARN_T2(time,str,str2)
  #define LOG_FAIL_T2(time,str,str2)
#endif


#endif