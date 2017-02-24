#ifndef MiiSerialModem_h
#define MiiSerialModem_h

#include <MiiGenericModem.h>

// The length of the headers we add.
#define SERIAL_HEADER_LEN 6

// This is the maximum number of bytes that can be carried.
#define SERIAL_MAX_PAYLOAD_LEN MII_RF_MESSAGE_LEN + SERAIL_HEADER_LEN

class MiiSerialModem : public MiiGenericModem {
public:

   MiiSerialModem(uint8_t selectPin = MII_PIN_RF_CS, uint8_t intPin = 0xFF, uint8_t sdnPin = 0xFF);

   bool  init(uint8_t address,uint16_t syncWords=0x2dd4,uint8_t channel=1,uint8_t power=MII_TXPOW_17DBM,bool isMaster=false,ModemConfigChoice index=Bw125Cr45Sf128);

    /// Turns the receiver on if it not already on.
    /// If there is a valid message available, copy it to buf and return true
    /// else return false.
    /// If a message is copied, *len is set to the length (Caution, 0 length messages are permitted).
    /// You should be sure to call this function frequently enough to not miss any messages
    /// It is recommended that you call it in your main loop.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
    bool    internalRecv(uint8_t* buf, uint8_t* len,bool peek=false);

    /// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
    /// Then loads a message into the transmitter and starts the transmitter. Note that a message length
    /// of 0 is permitted.
    /// \param[in] data Array of data to be sent
    /// \param[in] len Number of bytes of data to send
    /// \return true if the message length was valid and it was correctly queued for transmit
    bool  send(const uint8_t* data, uint8_t len);

    void setSyncWords(uint16_t syncWords);

protected:
    uint16_t _syncWords;
    uint16_t _rxHeaderSyncWords;

    /// Number of octets in the buffer
    volatile uint8_t    _bufLen;

    /// The receiver/transmitter buffer
    uint8_t             _buf[SERIAL_MAX_PAYLOAD_LEN];

};

#define MII_MODEM_CLASS MiiSerialModem

#endif
