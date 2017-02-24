#ifndef MiiPrinter_h
#define MiiPrinter_h 1

#include <Mii.h>
#include <Time.h>

#define BT_READY 100

typedef struct {
  char address[20];
} btaddress_t, *btaddress_ptr;

//----------------------------------------------------------------------
//Constants the BlueTooth
//----------------------------------------------------------------------
#ifndef MII_PRINTER_BAUD
#define MII_PRINTER_BAUD 38400
#endif
#define MII_BLUETOOTH_CMD_WAIT 1000

#define BT_SEARCH_COUNT 5
#define BT_SEARCH_TIME 5

class MiiPrinter : public Print {
public:
  MiiPrinter(Stream &stream=Serial);
  void clearBuf();
  virtual size_t write(uint8_t);
  bool    isConnected();
  bool    connect();
  void    reconnect();
  void    tab(uint16_t pos);
  void    line(uint16_t len);
  void    time(uint32_t t,bool rounding=true);
  void    date(time_t date,bool time=false);
  virtual void onConnected(){}
  virtual void onError();

  bool begin(const char* name,uint32_t pinCode);
  virtual void doProcess();

protected:
  uint8_t _step;
  uint32_t _wait;
  uint16_t _printPos;
  uint32_t _pinCode;
  uint8_t _listPos;
  char _name[15];
  btaddress_t _list[BT_SEARCH_COUNT];
  Stream &serial;
};

#endif