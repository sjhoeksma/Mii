#ifndef MiiRF_h
#define MiiRF_h
/**
MiiRF extends the RadioHead library with options is needed to do time
synchronization between 2 or devices. It also implements a own Acknowledge
system and buffering of collision messages.

In future we should see if we can use  uint32_t getLastPreambleTime() instead of _rxTime
*/
#include <Mii.h>

//Select one of the classes we support
#include <RH_RF22.h>
//#include <RH_RF24.h>

//Time used to send next TimeValidation
#define MII_TIME_INTERVAL_CLIENT 30000
//Time used to request time if not synced
#define MII_TIME_INTERVAL_MASTER 7500

//We used default 5 retries, to ensure data will arrive
#define MII_RF_DEFAULT_RETRY 3

//We accept max 10ms as drift between to times
#define MII_RF_DEFAULT_DRIFT 10

//Time sync is the last command
#define MII_C_TIMESYNC  0xFF
//Time send command
#define MII_C_TIME      0xFE
//Ack send command
#define MII_C_ACK       0xFD

// The acknowledgement bit in the FLAGS
#define MII_RF_FLAGS_ACK 0x80

//Redefine RH to MII
#define MII_BROADCAST_ADDRESS RH_BROADCAST_ADDRESS

//Create a unique name for each class and set constants depending on types
#if defined(RH_RF22_h)
#define MII_RF_EXT RH_RF22
#define MII_TXPOW_1DBM                         0x00
#define MII_TXPOW_2DBM                         0x01
#define MII_TXPOW_5DBM                         0x02
#define MII_TXPOW_8DBM                         0x03
#define MII_TXPOW_11DBM                        0x04
#define MII_TXPOW_14DBM                        0x05
#define MII_TXPOW_17DBM                        0x06
#define MII_TXPOW_20DBM                        0x07
#define MII_RF_MAX_MESSAGE_LEN RH_RF22_MAX_MESSAGE_LEN
//The minimum time between messages
#define MII_RF_MIN_MSG_INTERVAL 65

#elif defined(RH_RF24_h)
#define MII_RF_EXT RH_RF24
#define MII_TXPOW_1DBM                         0x00
#define MII_TXPOW_2DBM                         0x0f
#define MII_TXPOW_5DBM                         0x10
#define MII_TXPOW_8DBM                         0x1f
#define MII_TXPOW_11DBM                        0x28
#define MII_TXPOW_14DBM                        0x2f
#define MII_TXPOW_17DBM                        0x3f
#define MII_TXPOW_20DBM                        0x4f
#define MII_RF_MAX_MESSAGE_LEN RH_RF24_MAX_MESSAGE_LEN
//The minimum time between messages
#define MII_RF_MIN_MSG_INTERVAL 65

#else
#error No supported library
#endif

//Setting used rf22.FSK_Rb2Fd5 transmit time is 65 ms so we take 3 times just to be sure
#define MII_RF_DEF_TIMEOUT MII_RF_MIN_MSG_INTERVAL*3


//Check if the message size is big enough to receive timing_t message ()
#if  MII_SIZEOF_timing_t > MII_RF_MAX_MESSAGE_LEN
 #if defined(RH_RF22_h)
    #error The RH_RF22_MAX_MESSAGE_LEN (RH_RF22.h)is to small to support timing message
 #elif defined(RH_RF24_h)
    #error The RH_RF24_MAX_MESSAGE_LEN (RH_RF24.h)is to small to support timing message
 #else
   #error The MII_RF_MAX_MESSAGE_LEN is to small to support timing message
 #endif
#endif


//We support maximum 2 collision messages, setting to 0 will disable and save 500bytes
#define MII_MAX_COLLISIONS 2

//We support maximum 12 address connected to, setting 0 will disable address caching
#define MII_MAX_ADDRESS   12
//The default live span of a address we have seen in msseconds
#define MII_ADDRESS_LIVESPAN  600000L

//Struct for storing collision data
typedef struct {
  uint8_t headerId;
  uint8_t headerFrom;
  uint8_t headerTo;
  uint8_t headerFlags;
  uint8_t data[MII_RF_MAX_MESSAGE_LEN];
  uint8_t length;
} collisionRec_t, *collisionRec_ptr;

typedef struct {
  uint8_t  id; //The address we have seen
  uint32_t seen;    //When did we see this address and when
  uint8_t  rssi;    //What was the RSSI of this address
} addressRec_t, *addressRec_ptr;

class MiiRF : public MII_RF_EXT {
public:
  ///Init the based independent of RF device used
  MiiRF(uint8_t slaveSelectPin = MII_PIN_RF_CS, uint8_t interruptPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN, RHGenericSPI& spi = hardware_spi);

  /// Tests whether a new message is available from the Driver.
  /// \return true if a new, complete, error-free uncollected message is available to be retreived by recv().
  /// Internal processing of time sync is done here
  bool available();
  virtual bool available(bool allowCollision); //Allow collision to be returned


    /// Turns the receiver on if it not already on.
    /// If there is a valid message available, copy it to buf and return true
    /// else return false.
    /// If a message is copied, *len is set to the length (Caution, 0 length messages are permitted).
    /// You should be sure to call this function frequently enough to not miss any messages
    /// It is recommended that you call it in your main loop.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
  bool recv(uint8_t* buf=0, uint8_t* len=0);
  bool recv(bool allowCollision,uint8_t* buf=0, uint8_t* len=0);
  bool recv8_t(uint8_t  &buf,bool allowCollision=true);
  bool recv16_t(uint16_t  &buf,bool allowCollision=true);
  bool recv32_t(uint32_t  &buf,bool allowCollision=true);
  bool clearRecv(bool allowCollision=true);
  bool broadcast(){return _rxHeaderFrom==MII_BROADCAST_ADDRESS;}

  //Called when system needs to wait, will call YIELD
  virtual void idle(long time=0);

  bool send(const uint8_t* data, uint8_t len);

  //Send a command
  bool sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = MII_BROADCAST_ADDRESS,uint8_t flags=0);
  bool sendCmd(uint8_t cmd,uint32_t buf,uint8_t address = MII_BROADCAST_ADDRESS,uint8_t flags=0);
  bool sendCmd(uint8_t cmd,uint8_t address = MII_BROADCAST_ADDRESS,uint8_t flags=0);
  //Send a command which will be acknowledge
  bool sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
  bool sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
  bool sendAckCmd(uint8_t cmd,uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);


  //Wait for a command
  bool waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address = 0,uint16_t timeout = 0);
  bool waitForCmd(uint8_t cmd,uint8_t address = 0,uint16_t timeout =  0);

  bool sendTime(uint8_t toAddress);//Send the system time to a address
  uint32_t time(uint32_t _time=0);  //Get the time of system
  bool isMaster(); //Is this system master
  void setMaster(uint8_t address);//Set the master address
  void setRetries(uint8_t retries=MII_RF_DEFAULT_RETRY){_retryCount=retries;} //Set default retry sendAckCmd
  void setTimeout(uint16_t timeout=MII_RF_DEF_TIMEOUT){_timeout=timeout;} //Set default timeout for WaitForCmd
  void setAddress(uint8_t address);  //Set the internal address
  void setDrift(uint16_t drift){_drift=drift;}   //Set the time allowed to run appart in ms, before a full time sync will be done again
  void setSyncWords(const uint16_t syncWords);   //Easier way of setting the sync words
  void setTimeInterval(uint32_t interval){ _timeInterval = interval;} //Set the time update interval
  virtual bool syncTime(uint8_t address=MII_BROADCAST_ADDRESS);   //Sync the time with a address making  the address the master
  bool  init();   /// Overwrite init so we can switch on of power during init

   #if defined(RH_RF22_h)
    void  power_on_reset();  /// Cycles the Shutdown pin to force the cradio chip to reset

     /// A more easy init procedure
    bool init(uint8_t address,uint16_t syncWords=0x2dd4,uint8_t channel=1,uint8_t power=MII_TXPOW_17DBM,bool isMaster=false,ModemConfigChoice index=FSK_Rb2Fd5);
    void setChannel(uint8_t channel,uint8_t power=MII_TXPOW_17DBM,ModemConfigChoice index=FSK_Rb2Fd5);
   #elif defined(RH_RF24_h)
    /// A more easy init procedure
    bool init(uint8_t address,uint16_t syncWords=0x2dd4,uint8_t channel=1,uint8_t power=MII_TXPOW_17DBM,bool isMaster=false,ModemConfigChoice index=GFSK_Rb5Fd10);
    void setChannel(uint8_t channel,uint8_t power=MII_TXPOW_17DBM,ModemConfigChoice index=GFSK_Rb5Fd10);
   #endif

   #if MII_MAX_ADDRESS
   uint8_t nextAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the next address
   uint8_t prevAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   uint8_t firstAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   #endif

  boolean noTimeSync; //When set to true no active time sync will be done only on time commands of master


protected:
  //internalProcess allows you to do internal processing of commands during available checks
  virtual bool internalProcess(bool allowCollission);
  #if MII_MAX_COLLISIONS
  bool pushCollision(); //Push a received record on to collision store, true if record pushed
  bool popCollision(uint8_t* buf=0, uint8_t* len=0); //Pop a record from the collision store, true if record poped
  #endif

  bool      _inAvailable; //Flag used to lock available
  long      _timeDiff; //The difference between the master time and client time
  uint32_t  _ackTime; //Time when received message was acknowledge
  uint32_t  _airTime; //The time it takes one message to travel from device to device
  uint32_t  _txTime; //The time when last message was send
  uint8_t   _masterAddress; //The address of the master being used
  uint16_t  _drift; //The drift allowed between master and client before time is resynced
  uint32_t  _lastTime; //The last time the master send a time sync command
  uint16_t  _timeout; //The millis to wait when not time is specified
  uint8_t   _retryCount; //The default number of retries we should do
  uint32_t  _timeInterval; //Time interval to send next time

  #if defined(RH_RF22_h)
  uint8_t   _sdnPin;  /// The configured pin connected to the SDN pin of the radio
  #endif

  #if MII_MAX_COLLISIONS
  //Collision stuff
  uint8_t   _collisionCount; //The number of collisions in buffer
  collisionRec_t collisions[MII_MAX_COLLISIONS]; //The collision buffer
  #endif

  #if MII_MAX_ADDRESS
  uint8_t    _addressCount; //The number of addresses in the routing table
  addressRec_t   _address[MII_MAX_ADDRESS];  //The routing addresses
  #endif
};


#define MII_MODEM_CLASS MiiRF
#endif