#ifndef MiiGenericModem_h
#define MiiGenericModem_h

#include <Mii.h>
#include <SPI.h>

//The Max Message LEN
#define MII_MAX_MESSAGE_LEN 55

//Time used to send next TimeValidation
#define MII_TIME_INTERVAL_CLIENT 30000
//Time used to request time if not synced
#define MII_TIME_INTERVAL_MASTER 7500
//Time used to request time if we are using notimesync
#define MII_TIME_INTERVAL_NOTIMESYNC 4000

//The number of ms between two messages
#define MII_RF_MIN_MSG_INTERVAL 70

//We ping pong message so we use for time out 5 times the MSG interval
#define MII_RF_DEF_TIMEOUT MII_RF_MIN_MSG_INTERVAL*5

//Additionel time before sending again after send, allowing other systems to process
#define MII_RF_SEND_DELAY 15

//We used default 5 retries, to ensure data will arrive
#define MII_RF_DEFAULT_RETRY 3

//We accept max 10ms as drift between to times
#define MII_RF_DEFAULT_DRIFT 10

//We support maximum 2 collision messages, setting to 0 will disable and save 500bytes
#define MII_MAX_COLLISIONS 2

//We support maximum 8 address connected to, setting 0 will disable address caching
#define MII_MAX_ADDRESS   8
//The default live span of a address we have seen in msseconds
#define MII_ADDRESS_LIVESPAN  600000L

//The default broadcast address
#define MII_BROADCAST_ADDRESS 0xFF

//Time sync is the last command
#define MII_C_TIMESYNC  0xFF
//Time send command
#define MII_C_TIME      0xFE
//Ack send command
#define MII_C_ACK       0xFD
//Request Time command
#define MII_C_REQ_TIME  0xFC
//Request Change Channel, if set to none 0 value it will be enable
#define MII_C_CHANNEL  0x00

// The acknowledgement bit in the FLAGS
#define MII_RF_FLAGS_ACK 0x80
//Used to clear all flags
#define MII_ALL_FLAGS    0xFF

// This is the bit in the SPI address that marks it as a write
#define MII_RF_SPI_WRITE_MASK 0x80

// The message length
#define MII_RF_MAX_MESSAGE_LEN 55

// Keep track of the mode the RF22 is in
#define MII_RF_MODE_IDLE         0
#define MII_RF_MODE_RX           1
#define MII_RF_MODE_TX           2

#if MII_MAX_COLLISIONS
//Struct for storing collision data
typedef struct {
  uint8_t headerId;
  uint8_t headerFrom;
  uint8_t headerTo;
  uint8_t headerFlags;
  uint8_t data[MII_RF_MAX_MESSAGE_LEN];
  uint8_t length;
} collisionRec_t, *collisionRec_ptr;
#endif

#if MII_MAX_ADDRESS
typedef struct {
  uint8_t  id; //The address we have seen
  uint32_t seen;    //When did we see this address and when
  uint8_t  rssi;    //What was the RSSI of this address
} addressRec_t, *addressRec_ptr;
#endif


class MiiGenericModem
{
public:
    /// Constructor
    MiiGenericModem(uint8_t selectPin = MII_PIN_RF_CS, uint8_t intPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN);

    //Power own the device
    virtual void power(bool state);

    /// Initialise the Driver transport hardware and software.
    /// Make sure the Driver is properly configured before calling init().
    /// \return true if initialisation succeeded.
    virtual bool init();

    /// Turns the receiver on if it not already on.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
    virtual bool internalRecv(uint8_t* buf=0, uint8_t* len=0) = 0;

    /// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
    /// Then loads a message into the transmitter and starts the transmitter. Note that a message length
    /// of 0 is NOT permitted. If the message is too long for the underlying radio technology, send() will
    /// return false and will not send the message.
    /// \param[in] data Array of data to be sent
    /// \param[in] len Number of bytes of data to send (> 0)
    /// \return true if the message length was valid and it was correctly queued for transmit
    virtual bool send(const uint8_t* data=0, uint8_t len=0) = 0;

    /// Blocks until the transmitter
    /// is no longer transmitting.
    virtual bool            waitPacketSent(uint16_t timeout=0);

    /// Sets the address of this node. Defaults to 0xFF. Subclasses or the user may want to change this.
    /// This will be used to test the adddress in incoming messages. In non-promiscuous mode,
    /// only messages with a TO header the same as thisAddress or the broadcast addess (0xFF) will be accepted.
    /// In promiscuous mode, all messages will be accepted regardless of the TO header.
    /// In a conventional multinode system, all nodes will have a unique address
    /// (which you could store in EEPROM).
    /// You would normally set the header FROM address to be the same as thisAddress (though you dont have to,
    /// allowing the possibilty of address spoofing).
    /// \param[in] thisAddress The address of this node.
    virtual void           setAddress(uint8_t address);

    /// Sets the TO header to be sent in all subsequent messages
    /// \param[in] to The new TO header value
    virtual void           setHeaderTo(uint8_t to);

    /// Sets the FROM header to be sent in all subsequent messages
    /// \param[in] from The new FROM header value
    virtual void           setHeaderFrom(uint8_t from);

    /// Sets the ID header to be sent in all subsequent messages
    /// \param[in] id The new ID header value
    virtual void           setHeaderId(uint8_t id);

    /// Sets and clears bits in the FLAGS header to be sent in all subsequent messages.
    virtual void           setHeaderFlags(uint8_t set, uint8_t clear = MII_ALL_FLAGS);

    /// Tells the receiver to accept messages with any TO address, not just messages
    /// addressed to thisAddress or the broadcast address
    /// \param[in] promiscuous true if you wish to receive messages with any TO address
    virtual void           setPromiscuous(bool promiscuous);

    //Easier way of setting the sync words
    virtual void setSyncWords(const uint16_t syncWords){};

    /// If current mode is Rx or Tx changes it to Idle. If the transmitter or receiver is running,
    /// disables them.
    virtual void  setModeIdle(){};

    /// If current mode is Tx or Idle, changes it to Rx.
    virtual void  setModeRx(){};

    /// If current mode is Rx or Idle, changes it to Rx.
    virtual void  setModeTx(){};

    //internalProcess allows you to do internal processing of commands during available checks
    virtual bool internalProcess(bool allowCollission);

    //Set the channel we will be using
    virtual void setChannel(uint8_t channel){};

    //Set the transmitter and receiver centre frequency
    virtual bool setFrequency(float centre, float width = 0.05)=0;

    //Set the power
    virtual void setTxPower(uint8_t level){}


//--------- Common code, NO Virtual

  /// Turns the receiver on if it not already on.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
    bool recv(bool allowCollision,uint8_t* buf=0, uint8_t* len=0);

    /// Starts the receiver and blocks until a received message is available or a timeout
    /// \param[in] timeout Maximum time to wait in milliseconds.
    /// \return true if a message is available
    bool waitAvailable(uint16_t timeout=0);

    /// Tests whether a new message is available
    /// \return true if a new, complete, error-free uncollected message is available to be retreived by recv().
    bool available(bool allowCollision=true);

    //Send a command
    bool sendCmd(uint8_t cmd,uint8_t* buf=0, uint8_t len=0, uint8_t address = MII_BROADCAST_ADDRESS,uint8_t flags=0);
    //Send a command which will be acknowledge
    bool sendAckCmd(uint8_t cmd,uint8_t* buf=0, uint8_t len=0, uint8_t address = 0,uint8_t trys=0xFF,uint8_t flags=0);
    //Wait for a command
    bool waitForCmd(uint8_t cmd,uint8_t* buf=0, uint8_t len=0, uint8_t address = 0,uint16_t timeout = 0);
    //Discard a message
    bool discard(bool allowCollision=true);

    //Wait loop
    void idle(uint16_t timeout=0);

    /// Returns the TO header of the last received message
    /// \return The TO header
    uint8_t        headerTo();

    /// Returns the FROM header of the last received message
    /// \return The FROM header
    uint8_t        headerFrom();

    /// Returns the ID header of the last received message
    /// \return The ID header
    uint8_t        headerId();

    /// Returns the FLAGS header of the last received message
    /// \return The FLAGS header
    uint8_t        headerFlags();

    // Returns the RecieveTime
    uint32_t     rxTime();

    //returns the TransmitTime
    uint32_t     txTime();

    /// Returns the most recent RSSI (Receiver Signal Strength Indicator).
    /// Usually it is the RSSI of the last received message, which is measured when the preamble is received.
    /// If you called readRssi() more recently, it will return that more recent value.
    /// \return The most recent RSSI measurement in dBm.
    int8_t        lastRssi();

    /// Returns the operating mode of the library.
    /// \return the current mode
    uint8_t          mode();

    //Return true if we are ready
    bool isReady();

    //Sync the time between clients
    bool syncTime(uint8_t address=MII_BROADCAST_ADDRESS);
    uint32_t time(uint32_t _time=0);  //Get the time of system
    bool isMaster(); //Is this system master
    void setMaster(uint8_t address);//Set the master address
    void setRetries(uint8_t retries=MII_RF_DEFAULT_RETRY); //Set default retry sendAckCmd
    void setTimeout(uint16_t timeout=MII_RF_DEF_TIMEOUT);//Set default timeout for WaitForCmd
    void setDrift(uint16_t drift);  //Set the time allowed to run appart in ms, before a full time sync will be done again
    void setTimeInterval(uint32_t interval); //Set the time update interval
    void sendTime(); //Send the time
    void setTimeSync(bool state);

        /// \param[in] reg Register number, one of MII_RF_REG_*
    /// \return The value of the register
    uint8_t        spiRead(uint8_t reg);

    /// \param[in] reg Register number, one of MII_RF_REG_*
    /// \param[in] val The value to write
    void           spiWrite(uint8_t reg, uint8_t val);

    /// \param[in] reg Register number of the first register, one of MII_RF_REG_*
    /// \param[in] dest Array to write the register values to. Must be at least len bytes
    /// \param[in] len Number of bytes to read
    void           spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len);

    /// Write a number of consecutive registers using burst write mode
    /// \param[in] reg Register number of the first register, one of MII_RF_REG_*
    /// \param[in] src Array of new register values to write. Must be at least len bytes
    /// \param[in] len Number of bytes to write
    void           spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len);


   #if MII_MAX_ADDRESS
   uint8_t nextAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the next address
   uint8_t prevAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   uint8_t firstAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   #endif

   #if MII_C_CHANNEL
     void changeChannel(uint8_t channel); //Change the channel only master by sending command to all clients
   #endif

protected:
      /// Clears the receiver buffer.
    /// Internal use only
    virtual void  clearRxBuf()=0;

    //Check if modem has data avaiable
    virtual bool hasAvailable();

    /// The current transport operating mode
    volatile uint8_t    _mode;

    /// Indicator if buf isvalid
    volatile bool       _rxBufValid;


    /// This node id
    uint8_t             _thisAddress;

    /// Whether the transport is in promiscuous mode
    bool                _promiscuous;

    /// TO header in the last received mesasge
    volatile uint8_t    _rxHeaderTo;

    /// FROM header in the last received mesasge
    volatile uint8_t    _rxHeaderFrom;

    /// ID header in the last received mesasge
    volatile uint8_t    _rxHeaderId;

    /// FLAGS header in the last received messasge
    volatile uint8_t    _rxHeaderFlags;


    /// TO header to send in all messages
    uint8_t             _txHeaderTo;

    /// FROM header to send in all messages
    uint8_t             _txHeaderFrom;

    /// ID header to send in all messages
    uint8_t             _txHeaderId;

    /// FLAGS header to send in all messages
    uint8_t             _txHeaderFlags;

    /// The value of the last received RSSI value, in some transport specific units
    volatile int8_t     _lastRssi;

    // The time of the last received message
    volatile uint32_t   _rxTime;

    // The time of the last transmitted message
    uint32_t            _txTime;

    // Should we do a time sync
    bool                _timeSync;


    //The modem select pin
    uint8_t _selectPin;
    //The modem interrupt pin
    uint8_t _intPin;
    //The modem enable pin
    uint8_t _sdnPin;

    //The ready flag
    bool _ready;

    bool      _inAvailable; //Flag used to lock available
    long      _timeDiff; //The difference between the master time and client time
    uint32_t  _ackTime; //Time when received message was acknowledge
    uint32_t  _airTime; //The time it takes one message to travel from device to device
    uint8_t   _masterAddress; //The address of the master being used
    uint16_t  _drift; //The drift allowed between master and client before time is resynced
    uint32_t  _lastTime; //The last time the master send a time sync command
    uint16_t  _timeout; //The millis to wait when not time is specified
    uint8_t   _retryCount; //The default number of retries we should do
    uint32_t  _timeInterval; //Time interval to send next time

    //Make the select pin high
    void chipSelectHigh(void);
    //Make the select pin low
    void chipSelectLow(void);

    //SPI Flags to be stored
    #if SPI_ISOLATE
    uint8_t             _SPCR;
    uint8_t             _SPSR;
    uint8_t             _SPCRO;
    uint8_t             _SPSRO;
    #endif

    #if MII_MAX_COLLISIONS
     //Collision stuff
     uint8_t   _collisionCount; //The number of collisions in buffer
     collisionRec_t collisions[MII_MAX_COLLISIONS]; //The collision buffer
     bool pushCollision(); //Push a received record on to collision store, true if record pushed
     bool popCollision(uint8_t* buf=0, uint8_t* len=0); //Pop a record from the collision store, true if record poped
    #endif

   #if MII_MAX_ADDRESS
    uint8_t    _addressCount; //The number of addresses in the routing table
    addressRec_t   _address[MII_MAX_ADDRESS];  //The routing addresses
   #endif

};


#endif
