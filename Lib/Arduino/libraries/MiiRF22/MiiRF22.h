#ifndef MiiRF22_h
#define MiiRF22_h


#include <RF22.h>

//Bit functions clearBit and setBit
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#ifndef vbi
#define vbi(sfr,bit) (_SFR_BYTE(sfr) & _BV(bit))
#endif


//Should we add id to our messages
#define MII_RF22_ID  0

//Time used to send next TimeValidation
#define MII_LASTTIME_INTERVAL_SEND 8512
//Time used to request time if not synced
#define MII_LASTTIME_INTERVAL_RECV 2500

//Setting used rf22.FSK_Rb2Fd5 transmit time is 65 ms so we take 250 just to be sure
#define MII_DEF_TIMEOUT 250

#define MII_BROADCAST   RF22_BROADCAST_ADDRESS
//Time sync is the last command
#define MII_C_TIMESYNC  0xFF
//Time send command
#define MII_C_TIME      0xFE
//Acknowledge command
#define MII_C_ACK       0xFD
//Time sync search command, remote system will respond, if synced, based on there id as delay with MII_C_TIMESYNC
#define MII_C_TIMESYNC_SEARCH 0xFC

//Set to 1 if we should use software SPI
#define RF22_SOFT_SPI 0

#define RF22_LOCK_AVAILABLE 0
// The acknowledgement bit in the FLAGS
#define RF22_FLAGS_ACK 0x80
// The pressant bit in the FLAGS
#define RF22_FLAGS_CON 0x40
// The data passed should be route to specific or if broadcast to all
#define RF22_FLAGS_ID 0x20
// The data passed should be route to specific or if broadcast to all
#define RF22_FLAGS_OTHER 0x10

//The max time in ms which is allowed for the clock to drift in one cycle before we restart time sync
#define MII_MAX_DRIFT 10

//Number of default retries
#define RF22_RETRY_COUNT 3


class MiiRF22  :  public RF22 {
public :
    MiiRF22(uint8_t thisAddress, uint8_t slaveSelectPin, uint8_t interrupt,uint16_t drift=MII_MAX_DRIFT);
    boolean init(boolean isMaster=false);
    virtual boolean available();
    void setAddress(uint8_t thisAddress);
    //Send a command to a specific address
    boolean sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = MII_BROADCAST,uint8_t flags=0);
    boolean sendCmd(uint8_t cmd,uint32_t buf,uint8_t address = MII_BROADCAST,uint8_t flags=0);
    boolean sendCmd(uint8_t cmd,uint8_t address = MII_BROADCAST,uint8_t flags=0);
    boolean sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
    boolean sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
    boolean sendAckCmd(uint8_t cmd,uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
    bool recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from = NULL, uint8_t* to = NULL, uint8_t* id = NULL, uint8_t* flags = NULL);
    //Wait for a command -1 timeout otherwise length of data received
    int waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = MII_BROADCAST,uint16_t timeout = 0);
    boolean waitForCmd(uint8_t cmd,uint8_t address = MII_BROADCAST,uint16_t timeout =  0);
    boolean sendTime(uint8_t toAddress);
    uint32_t time(uint32_t _time=0){return isMaster() ? (_time ? _time : millis()) : (_timeDiff ? (_time ? _time : millis())+ _timeDiff : 0 );}
    //Get the master address if not available do time sync to collect it
    uint8_t getMasterAddress();
    boolean isAddress(uint8_t address){return _thisAddress==address;}
    boolean isMaster(){return _masterAddress==_thisAddress;}
    void setMaster(uint8_t address){_masterAddress=address;}
    void setRetries(uint8_t retries=RF22_RETRY_COUNT){_retryCount=retries;}
    void setTimeout(uint16_t timeout=MII_DEF_TIMEOUT){_timeout=timeout;}
    void clearRx(){clearRxBuf();}
    void forceSyncTime(){_lastTime=0;}
    void setDrift(uint16_t drift){_drift=drift;}
    void setChannel(uint8_t channel,uint8_t power=RF22_TXPOW_17DBM,ModemConfigChoice index=FSK_Rb2Fd5);
    void setSyncWords(const uint16_t syncWords);

    uint8_t _syncCount;

protected:
    long  _timeDiff;
    uint32_t _time;
    uint16_t _airTime;
    uint8_t _thisAddress;
    uint8_t _masterAddress;
    uint8_t _lock;
    uint16_t _timeout;
    uint32_t _lastTime;
    uint8_t  _retryCount;
    uint32_t _ackTime;
    uint8_t _msgId;
    uint16_t _drift;
    uint16_t _syncwords;

 private:
    uint32_t T[6]; //The times used for time sync

};

#endif