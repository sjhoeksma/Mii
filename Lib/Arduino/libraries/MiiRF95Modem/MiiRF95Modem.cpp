#include <MiiRF95Modem.h>

#if defined MPIDE
#include <peripheral/int.h>
#define memcpy_P memcpy
#define ATOMIC_BLOCK_START unsigned int __status = INTDisableInterrupts(); {
#define ATOMIC_BLOCK_END } INTRestoreInterrupts(__status);
#elif defined ARDUINO
#include <util/atomic.h>
#define ATOMIC_BLOCK_START     ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#define ATOMIC_BLOCK_END }
#endif


// Interrupt vectors for the 3 Arduino interrupt pins
// Each interrupt can be handled by a different instance of RH_RF95, allowing you to have
// 2 or more LORAs per Arduino
MiiRF95Modem* MiiRF95Modem::_deviceForInterrupt= 0;
// These are indexed by the values of ModemConfigChoice
// Stored in flash (program) memory to save SRAM
PROGMEM static const MiiRF95Modem::ModemConfig MODEM_CONFIG_TABLE[] =
{
    //  1d,     1e,      26
    { 0x72,   0x74,    0x00}, // Bw125Cr45Sf128 (the chip default)
    { 0x92,   0x74,    0x00}, // Bw500Cr45Sf128
    { 0x48,   0x94,    0x00}, // Bw31_25Cr48Sf512
    { 0x78,   0xc4,    0x00}, // Bw125Cr48Sf4096

};

MiiRF95Modem::MiiRF95Modem(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
    : MiiGenericModem(selectPin, intPin, sdnPin),
    _syncWords(0){
}

bool MiiRF95Modem::init(uint8_t address,uint16_t syncWords,uint8_t channel,uint8_t power,bool isMaster,ModemConfigChoice index){
    if (!MiiGenericModem::init()) return false;

    // Determine the interrupt number that corresponds to the interruptPin
    int interruptNumber = digitalPinToInterrupt(_intPin);
    if (interruptNumber == NOT_AN_INTERRUPT) return false;

    // No way to check the device type :-(

    // Set sleep mode, so we can also set LORA mode:
    spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);
    delay(10); // Wait for sleep mode to take over from say, CAD
    // Check we are in sleep mode, with LORA set
    if (spiRead(RH_RF95_REG_01_OP_MODE) != (RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE)) {
      //	Serial.println(spiRead(RH_RF95_REG_01_OP_MODE), HEX);
	   return false; // No device present?
    }

    // Set up interrupt handler
    // Since there are a limited number of interrupt glue functions isr*() available,
    // we can only support a limited number of devices simultaneously
    // ON some devices, notably most Arduinos, the interrupt pin passed in is actuallt the
    // interrupt number. You have to figure out the interruptnumber-to-interruptpin mapping
    // yourself based on knwledge of what Arduino board you are running on.
    _deviceForInterrupt = this;
    attachInterrupt(interruptNumber, isr0, FALLING);


    // Set up FIFO
    // We configure so that we can use the entire 256 byte FIFO for either receive
    // or transmit, but not both at the same time
    spiWrite(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spiWrite(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    // Packet format is preamble + explicit-header + payload + crc
    // Explicit Header Mode
    // payload is TO + FROM + ID + FLAGS + message data
    // RX mode is implmented with RXCONTINUOUS
    // max message data length is 255 - 4 = 251 octets

    setModeIdle();

    // Set up default configuration
    // No Sync Words in LORA mode.
    setModemConfig(index); // Radio default
//    setModemConfig(Bw125Cr48Sf4096); // slow and reliable?
    setPreambleLength(8); // Default is 8

    setAddress(address);
    setSyncWords(syncWords);
    #ifdef MII_RF_FREQ_FIXED
      setFrequency(MII_RF_FREQ_FIXED, MII_RF_CHANNEL_WIDTH);
    #else
     setChannel(channel);
    #endif
    setTxPower(power);

    return true;
}

void MiiRF95Modem::setSyncWords(uint16_t syncWords){
  _syncWords = syncWords;
}

// C++ level interrupt handler for this instance
// LORA is unusual in that it has several interrupt lines, and not a single, combined one.
// On MiniWirelessLoRa, only one of the several interrupt lines (DI0) from the RFM95 is usefuly
// connnected to the processor.
// We use this to get RxDone and TxDone interrupts
void MiiRF95Modem::handleInterrupt(){
    // Read the interrupt register
    uint8_t irq_flags = spiRead(RH_RF95_REG_12_IRQ_FLAGS);
    if (_mode == MII_RF_MODE_RX && irq_flags & (RH_RF95_RX_TIMEOUT | RH_RF95_PAYLOAD_CRC_ERROR)) {
	  } else if (_mode == MII_RF_MODE_RX && irq_flags & RH_RF95_RX_DONE) {
	    // Have received a packet
	    uint8_t len = spiRead(RH_RF95_REG_13_RX_NB_BYTES);

	    // Reset the fifo read ptr to the beginning of the packet
	    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, spiRead(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR));
	    spiBurstRead(RH_RF95_REG_00_FIFO, _buf, len);
	    _bufLen = len;
	    spiWrite(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags

	    // Remember the RSSI of this packet
	    // this is according to the doc, but is it really correct?
	    // weakest receiveable signals are reported RSSI at about -66
	    _lastRssi = spiRead(RH_RF95_REG_1A_PKT_RSSI_VALUE) - 137;

	    // We have received a message.
	    validateRxBuf();
	    if (_rxBufValid) setModeIdle(); // Got one
    } else if (_mode == MII_RF_MODE_TX && irq_flags & RH_RF95_TX_DONE) {
	    setModeIdle();
    }
    spiWrite(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // Clear all IRQ flags
}

// These are low level functions that call the interrupt handler for the correct
// instance of RH_RF95.
// 3 interrupts allows us to have 3 different devices
void MiiRF95Modem::isr0(){
	_deviceForInterrupt->handleInterrupt();
}
// Check whether the latest received message is complete and uncorrupted
void MiiRF95Modem::validateRxBuf() {
    if (_bufLen <  RH_RF95_HEADER_LEN) return; // Too short to be a real message
    // Extract the 6 headers
    _rxHeaderTo    = _buf[0];
    _rxHeaderFrom  = _buf[1];
    _rxHeaderId    = _buf[2];
    _rxHeaderFlags = _buf[3];
    _rxHeaderSyncWords  = *(uint16_t*)&_buf[4];
    if (_syncWords== _rxHeaderSyncWords || _promiscuous || _rxHeaderTo == _thisAddress || _rxHeaderTo == MII_BROADCAST_ADDRESS) {
   	  _rxTime = millis();
    	_rxBufValid = true;
    }
}

void MiiRF95Modem::clearRxBuf(){
    ATOMIC_BLOCK_START;
    _rxBufValid = false;
    _bufLen = 0;
    ATOMIC_BLOCK_END;
}

bool MiiRF95Modem::internalRecv(uint8_t* buf, uint8_t* len){
  if (!hasAvailable()) return false;
  if (buf && len) {
    ATOMIC_BLOCK_START;
    // Skip the 4 headers that are at the beginning of the rxBuf
    if (*len > _bufLen-RH_RF95_HEADER_LEN)
        *len = _bufLen-RH_RF95_HEADER_LEN;
    memcpy(buf, _buf+RH_RF95_HEADER_LEN, *len);
    ATOMIC_BLOCK_END;
  }
  clearRxBuf(); // This message accepted and cleared
  return true;
}

bool MiiRF95Modem::send(const uint8_t* data, uint8_t len){
  if (len > RH_RF95_MAX_MESSAGE_LEN) return false;
  waitPacketSent(); // Make sure we dont interrupt an outgoing message
  setModeIdle();

  // Position at the beginning of the FIFO
  spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);
  // The headers
  spiWrite(RH_RF95_REG_00_FIFO, _txHeaderTo);
  spiWrite(RH_RF95_REG_00_FIFO, _txHeaderFrom);
  spiWrite(RH_RF95_REG_00_FIFO, _txHeaderId);
  spiWrite(RH_RF95_REG_00_FIFO, _txHeaderFlags);
  spiBurstWrite(RH_RF95_REG_00_FIFO, (uint8_t*)&_syncWords, sizeof(uint16_t));
  // The message data
  spiBurstWrite(RH_RF95_REG_00_FIFO, data, len);
  spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, len + RH_RF95_HEADER_LEN);

  setModeTx(); // Start the transmitter
  // when Tx is done, interruptHandler will fire and radio mode will return to STANDBY
  return true;
}

bool MiiRF95Modem::setFrequency(float centre,float width){
    // Frf = FRF / FSTEP
    uint32_t frf = (centre * 1000000.0) / RH_RF95_FSTEP;
    spiWrite(RH_RF95_REG_06_FRF_MSB, (frf >> 16) & 0xff);
    spiWrite(RH_RF95_REG_07_FRF_MID, (frf >> 8) & 0xff);
    spiWrite(RH_RF95_REG_08_FRF_LSB, frf & 0xff);
    return true;
}

void MiiRF95Modem::setModeIdle(){
  if (_mode != MII_RF_MODE_IDLE) {
	  spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);
	  _mode = MII_RF_MODE_IDLE;
  }
}

void MiiRF95Modem::setModeRx(){
  if (_mode != MII_RF_MODE_RX) {
	spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_RXCONTINUOUS);
	spiWrite(RH_RF95_REG_40_DIO_MAPPING1, 0x00); // Interrupt on RxDone
	_mode = MII_RF_MODE_RX;
  }
}

void MiiRF95Modem::setModeTx(){
  if (_mode != MII_RF_MODE_TX){
    spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_TX);
    spiWrite(RH_RF95_REG_40_DIO_MAPPING1, 0x40); // Interrupt on TxDone
    _mode = MII_RF_MODE_TX;
  }
}

void MiiRF95Modem::setTxPower(int8_t power){
  if (power > 23)
	  power = 23;
  if (power < 5)
	  power = 5;

    // For RH_RF95_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
    // RH_RF95_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
    // for 21, 22 and 23dBm
  if (power > 20) {
	  spiWrite(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_ENABLE);
	  power -= 3;
  } else {
	  spiWrite(RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_DISABLE);
  }

    // RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
    // pin is connected, so must use PA_BOOST
    // Pout = 2 + OutputPower.
    // The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
    // but OutputPower claims it would be 17dBm.
    // My measurements show 20dBm is correct
    spiWrite(RH_RF95_REG_09_PA_CONFIG, RH_RF95_PA_SELECT | (power-5));
}

// Sets registers from a canned modem configuration structure
void MiiRF95Modem::setModemRegisters(const ModemConfig* config){
    spiWrite(RH_RF95_REG_1D_MODEM_CONFIG1,       config->reg_1d);
    spiWrite(RH_RF95_REG_1E_MODEM_CONFIG2,       config->reg_1e);
    spiWrite(RH_RF95_REG_26_MODEM_CONFIG3,       config->reg_26);
}

// Set one of the canned FSK Modem configs
// Returns true if its a valid choice
bool MiiRF95Modem::setModemConfig(ModemConfigChoice index){
    if (index > (signed int)(sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))
        return false;

    ModemConfig cfg;
    memcpy_P(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(MiiRF95Modem::ModemConfig));
    setModemRegisters(&cfg);
    return true;
}

void MiiRF95Modem::setPreambleLength(uint16_t bytes){
    spiWrite(RH_RF95_REG_20_PREAMBLE_MSB, bytes >> 8);
    spiWrite(RH_RF95_REG_21_PREAMBLE_LSB, bytes & 0xff);
}

