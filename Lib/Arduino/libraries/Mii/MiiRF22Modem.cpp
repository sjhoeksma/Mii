#include <MiiRF22Modem.h>

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


// These are indexed by the values of ModemConfigChoice
// Canned modem configurations generated with
// http://www.hoperf.com/upload/rf/RF22B%2023B%2031B%2042B%2043B%20Register%20Settings_RevB1-v5.xls
// Stored in flash (program) memory to save SRAM
#ifndef MII_RF_FIXED_CONFIG
PROGMEM static const MiiRF22Modem::ModemConfig MODEM_CONFIG_TABLE[] = {
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x00, 0x08 }, // Unmodulated carrier
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x33, 0x08 }, // FSK, PN9 random modulation, 2, 5

    // All the following enable FIFO with reg 71
    //  1c,   1f,   20,   21,   22,   23,   24,   25,   2c,   2d,   2e,   58,   69,   6e,   6f,   70,   71,   72
    // FSK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x22, 0x08 }, // 2, 5
    { 0x1b, 0x03, 0x41, 0x60, 0x27, 0x52, 0x00, 0x07, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x22, 0x3a }, // 2.4, 36
    { 0x1d, 0x03, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x13, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x22, 0x48 }, // 4.8, 45
    { 0x1e, 0x03, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x45, 0x40, 0x0a, 0x20, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x22, 0x48 }, // 9.6, 45
    { 0x2b, 0x03, 0x34, 0x02, 0x75, 0x25, 0x07, 0xff, 0x40, 0x0a, 0x1b, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x22, 0x0f }, // 19.2, 9.6
    { 0x02, 0x03, 0x68, 0x01, 0x3a, 0x93, 0x04, 0xd5, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x22, 0x1f }, // 38.4, 19.6
    { 0x06, 0x03, 0x45, 0x01, 0xd7, 0xdc, 0x07, 0x6e, 0x40, 0x0a, 0x2d, 0x80, 0x60, 0x0e, 0xbf, 0x0c, 0x22, 0x2e }, // 57.6. 28.8
    { 0x8a, 0x03, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x40, 0x0a, 0x50, 0x80, 0x60, 0x20, 0x00, 0x0c, 0x22, 0xc8 }, // 125, 125

    { 0x2b, 0x03, 0xa1, 0xe0, 0x10, 0xc7, 0x00, 0x09, 0x40, 0x0a, 0x1d,  0x80, 0x60, 0x04, 0x32, 0x2c, 0x22, 0x04 }, // 512 baud, FSK, 2.5 Khz fd for POCSAG compatibility
    { 0x27, 0x03, 0xa1, 0xe0, 0x10, 0xc7, 0x00, 0x06, 0x40, 0x0a, 0x1d,  0x80, 0x60, 0x04, 0x32, 0x2c, 0x22, 0x07 }, // 512 baud, FSK, 4.5 Khz fd for POCSAG compatibility

    // GFSK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    // These differ from FSK only in register 71, for the modulation type
    { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x23, 0x08 }, // 2, 5
    { 0x1b, 0x03, 0x41, 0x60, 0x27, 0x52, 0x00, 0x07, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x23, 0x3a }, // 2.4, 36
    { 0x1d, 0x03, 0xa1, 0x20, 0x4e, 0xa5, 0x00, 0x13, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x23, 0x48 }, // 4.8, 45
    { 0x1e, 0x03, 0xd0, 0x00, 0x9d, 0x49, 0x00, 0x45, 0x40, 0x0a, 0x20, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x23, 0x48 }, // 9.6, 45
    { 0x2b, 0x03, 0x34, 0x02, 0x75, 0x25, 0x07, 0xff, 0x40, 0x0a, 0x1b, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x23, 0x0f }, // 19.2, 9.6
    { 0x02, 0x03, 0x68, 0x01, 0x3a, 0x93, 0x04, 0xd5, 0x40, 0x0a, 0x1e, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x23, 0x1f }, // 38.4, 19.6
    { 0x06, 0x03, 0x45, 0x01, 0xd7, 0xdc, 0x07, 0x6e, 0x40, 0x0a, 0x2d, 0x80, 0x60, 0x0e, 0xbf, 0x0c, 0x23, 0x2e }, // 57.6. 28.8
    { 0x8a, 0x03, 0x60, 0x01, 0x55, 0x55, 0x02, 0xad, 0x40, 0x0a, 0x50, 0x80, 0x60, 0x20, 0x00, 0x0c, 0x23, 0xc8 }, // 125, 125

    // OOK, No Manchester, Max Rb err <1%, Xtal Tol 20ppm
    { 0x51, 0x03, 0x68, 0x00, 0x3a, 0x93, 0x01, 0x3d, 0x2c, 0x11, 0x28, 0x80, 0x60, 0x09, 0xd5, 0x2c, 0x21, 0x08 }, // 1.2, 75
    { 0xc8, 0x03, 0x39, 0x20, 0x68, 0xdc, 0x00, 0x6b, 0x2a, 0x08, 0x2a, 0x80, 0x60, 0x13, 0xa9, 0x2c, 0x21, 0x08 }, // 2.4, 335
    { 0xc8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x29, 0x04, 0x29, 0x80, 0x60, 0x27, 0x52, 0x2c, 0x21, 0x08 }, // 4.8, 335
    { 0xb8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x82, 0x29, 0x80, 0x60, 0x4e, 0xa5, 0x2c, 0x21, 0x08 }, // 9.6, 335
    { 0xa8, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x41, 0x29, 0x80, 0x60, 0x9d, 0x49, 0x2c, 0x21, 0x08 }, // 19.2, 335
    { 0x98, 0x03, 0x9c, 0x00, 0xd1, 0xb7, 0x00, 0xd4, 0x28, 0x20, 0x29, 0x80, 0x60, 0x09, 0xd5, 0x0c, 0x21, 0x08 }, // 38.4, 335
    { 0x98, 0x03, 0x96, 0x00, 0xda, 0x74, 0x00, 0xdc, 0x28, 0x1f, 0x29, 0x80, 0x60, 0x0a, 0x3d, 0x0c, 0x21, 0x08 }, // 40, 335

};
#endif

MiiRF22Modem::MiiRF22Modem(uint8_t selectPin, uint8_t intPin, uint8_t sdnPin)
  : MiiGenericModem(selectPin,intPin,sdnPin) ,
  _idleMode(MII_RF_XTON) , // Default idle state is READY mode
  _polynomial(CRC_16_IBM) // Historical
{}


  //Power own the device
void MiiRF22Modem::powerOn(bool state){
  if (_sdnPin!=MII_NO_PIN) {
    //First turn off the modem
    pinMode(_sdnPin, OUTPUT);
    digitalWrite(_sdnPin, HIGH);
    delay(25);
    //Now Enable it again
    if (state) digitalWrite(_sdnPin, LOW);
  }
  delay(100);
}

/// A more easy init procedure
bool MiiRF22Modem::init(uint8_t address,uint16_t syncWords,uint8_t channel,uint8_t power,bool isMaster,ModemConfigChoice index){
    if (!MiiGenericModem::init()) return false;
    // Determine the interrupt number that corresponds to the interruptPin
    int interruptNumber = digitalPinToInterrupt(_intPin);
    if (interruptNumber == NOT_AN_INTERRUPT) return false;

    // Software reset the device
    reset();

    // Get the device type and check it
    // This also tests whether we are really connected to a device
    _deviceType = spiRead(MII_RF_REG_00_DEVICE_TYPE);
    if (   _deviceType != MII_RF_DEVICE_TYPE_RX_TRX
        && _deviceType != MII_RF_DEVICE_TYPE_TX) {
	    return false;
    }

     // Enable interrupts
    spiWrite(MII_RF_REG_05_INTERRUPT_ENABLE1, MII_RF_ENTXFFAEM | MII_RF_ENRXFFAFULL | MII_RF_ENPKSENT | MII_RF_ENPKVALID | MII_RF_ENCRCERROR | MII_RF_ENFFERR);
    spiWrite(MII_RF_REG_06_INTERRUPT_ENABLE2, MII_RF_ENPREAVAL);


    // Set up interrupt handler
    attachInterrupt(interruptNumber, isr, FALLING);

    setModeIdle();

    clearTxBuf();
    clearRxBuf();

    // Most of these are the POR default
    spiWrite(MII_RF_REG_7D_TX_FIFO_CONTROL2, MII_RF_TXFFAEM_THRESHOLD);
    spiWrite(MII_RF_REG_7E_RX_FIFO_CONTROL,  MII_RF_RXFFAFULL_THRESHOLD);
    spiWrite(MII_RF_REG_30_DATA_ACCESS_CONTROL, MII_RF_ENPACRX | MII_RF_ENPACTX | MII_RF_ENCRC | (_polynomial & MII_RF_CRC));


    // Configure the message headers
    // Here we set up the standard packet format for use by the RH_RF22 library
    // 8 nibbles preamble
    // 2 SYNC words 2d, d4
    // Header length 4 (to, from, id, flags)
    // 1 octet of data length (0 to 255)
    // 0 to 255 octets data
    // 2 CRC octets as CRC16(IBM), computed on the header, length and data
    // On reception the to address is check for validity against RH_RF22_REG_3F_CHECK_HEADER3
    // or the broadcast address of 0xff
    // If no changes are made after this, the transmitted
    // to address will be 0xff, the from address will be 0xff
    // and all such messages will be accepted. This permits the out-of the box
    // RH_RF22 config to act as an unaddresed, unreliable datagram service
    spiWrite(MII_RF_REG_32_HEADER_CONTROL1, MII_RF_BCEN_HEADER3 | MII_RF_HDCH_HEADER3);
    spiWrite(MII_RF_REG_33_HEADER_CONTROL2, MII_RF_HDLEN_4 | MII_RF_SYNCLEN_2);

    setPreambleLength(8);
    setPromiscuous(false);
    setGpioReversed(false);

    setAddress(address);
    setSyncWords(syncWords);
    setModemConfig(index);
    setChannel(channel);

    setTxPower(power);

    if (isMaster) setMaster(address);
    _ready=true;
 //   setModeRx();
    return true;
}

// C++ level interrupt handler for this instance
void MiiRF22Modem::handleInterrupt(){
    uint8_t _lastInterruptFlags[2];
    // Read the interrupt flags which clears the interrupt
    spiBurstRead(MII_RF_REG_03_INTERRUPT_STATUS1, _lastInterruptFlags, 2);

    if (_lastInterruptFlags[0] & MII_RF_IFFERROR) {
	     resetFifos(); // Clears the interrupt
	     if (_mode == MII_RF_MODE_TX)
	        restartTransmit();
	     else if (_mode == MII_RF_MODE_RX)
	    clearRxBuf();
    }
    // Caution, any delay here may cause a FF underflow or overflow
    if (_lastInterruptFlags[0] & MII_RF_ITXFFAEM) {
	    // See if more data has to be loaded into the Tx FIFO
	   sendNextFragment();
    }
    if (_lastInterruptFlags[0] & MII_RF_IRXFFAFULL) {
      	// Caution, any delay here may cause a FF overflow
	      // Read some data from the Rx FIFO
	      readNextFragment();
    }
    if (_lastInterruptFlags[0] & MII_RF_IEXT) {
    	// This is not enabled by the base code, but users may want to enable it
	    handleExternalInterrupt();
    }
    if (_lastInterruptFlags[1] & MII_RF_IWUT) {
	     // This is not enabled by the base code, but users may want to enable it
	     handleWakeupTimerInterrupt();
    }
    if (_lastInterruptFlags[0] & MII_RF_IPKSENT) {
	    // Transmission does not automatically clear the tx buffer.
	    // Could retransmit if we wanted
	    // RF22 transitions automatically to Idle
	    _mode = MII_RF_MODE_IDLE;
    }
    if (_lastInterruptFlags[0] & MII_RF_IPKVALID) {
	    uint8_t len = spiRead(MII_RF_REG_4B_RECEIVED_PACKET_LENGTH);

	    // May have already read one or more fragments
	    // Get any remaining unread octets, based on the expected length
	    // First make sure we dont overflow the buffer in the case of a stupid length
	    // or partial bad receives
	    if (   len >  MII_RF_MESSAGE_LEN || len < _bufLen){
	       _mode = MII_RF_MODE_IDLE;
	       clearRxBuf();
	      return; // Hmmm receiver buffer overflow.
	    }

	    spiBurstRead(MII_RF_REG_7F_FIFO_ACCESS, _buf + _bufLen, len - _bufLen);
	    _rxHeaderTo = spiRead(MII_RF_REG_47_RECEIVED_HEADER3);
	    _rxHeaderFrom = spiRead(MII_RF_REG_48_RECEIVED_HEADER2);
	    _rxHeaderId = spiRead(MII_RF_REG_49_RECEIVED_HEADER1);
	    _rxHeaderFlags = spiRead(MII_RF_REG_4A_RECEIVED_HEADER0);
	    _bufLen = len;
	    _mode = MII_RF_MODE_IDLE;
	    _rxTime = millis();
	    _rxBufValid = true;
	    #if MII_MAX_MSG_CACHE
       pushMsg();  //Push the received item on stack if implemented
      #endif

    }
    if (_lastInterruptFlags[0] & MII_RF_ICRCERROR) {
      clearRxBuf();
	    resetRxFifo();
	    _mode = MII_RF_MODE_IDLE;
	    setModeRx(); // Keep trying
    }
    if (_lastInterruptFlags[1] & MII_RF_IPREAVAL) {
	    _lastRssi =  (int8_t)(-120 + ((spiRead(MII_RF_REG_26_RSSI) / 2)));;
	    resetRxFifo();
	    clearRxBuf();
    }
}

// Default implmentation does nothing. Override if you wish
void MiiRF22Modem::handleExternalInterrupt(){
}

// Default implmentation does nothing. Override if you wish
void MiiRF22Modem::handleWakeupTimerInterrupt(){
}


void MiiRF22Modem::reset(){
    spiWrite(MII_RF_REG_07_OPERATING_MODE1, MII_RF_SWRES);
    // Wait for it to settle
    delay(1); // SWReset time is nominally 100usec
}

uint8_t MiiRF22Modem::statusRead(){
    return spiRead(MII_RF_REG_02_DEVICE_STATUS);
}

void MiiRF22Modem::setMode(uint8_t mode){
    spiWrite(MII_RF_REG_07_OPERATING_MODE1, mode);
}

void MiiRF22Modem::setModeIdle(){
  if (_mode != MII_RF_MODE_IDLE) {
	  setMode(_idleMode);
	  _mode = MII_RF_MODE_IDLE;
  }
}

void MiiRF22Modem::setModeRx(){
    if (_mode != MII_RF_MODE_RX) {
	    setMode(_idleMode | MII_RF_RXON);
	    _mode = MII_RF_MODE_RX;
   }
}

void MiiRF22Modem::setModeTx(){
  if (_mode != MII_RF_MODE_TX) {
	  setMode(_idleMode | MII_RF_TXON);
	  _mode = MII_RF_MODE_TX;
	  // Hmmm, if you dont clear the RX FIFO here, then it appears that going
	  // to transmit mode in the middle of a receive can corrupt the
	  // RX FIFO
	  resetRxFifo();
  }
}

void MiiRF22Modem::setTxPower(int8_t power){
  //We see a lot of problems when power is on 20 so limit it to 17
  power=min(map(power,MII_TXPOW_1DBM,MII_TXPOW_23DBM,MII_RF_TXPOW_1DBM,MII_RF_TXPOW_20DBM),MII_RF_TXPOW_17DBM);
  spiWrite(MII_RF_REG_6D_TX_POWER, (uint8_t)power);
}

// Sets registers from a canned modem configuration structure
void MiiRF22Modem::setModemRegisters(const ModemConfig* config){
    spiWrite(MII_RF_REG_1C_IF_FILTER_BANDWIDTH,                    config->reg_1c);
    spiWrite(MII_RF_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE,      config->reg_1f);
    spiBurstWrite(MII_RF_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE, &config->reg_20, 6);
    spiBurstWrite(MII_RF_REG_2C_OOK_COUNTER_VALUE_1,              &config->reg_2c, 3);
    spiWrite(MII_RF_REG_58_CHARGE_PUMP_CURRENT_TRIMMING,           config->reg_58);
    spiWrite(MII_RF_REG_69_AGC_OVERRIDE1,                          config->reg_69);
    spiBurstWrite(MII_RF_REG_6E_TX_DATA_RATE1,                    &config->reg_6e, 5);
}

// Set one of the canned FSK Modem configs
// Returns true if its a valid choice
bool MiiRF22Modem::setModemConfig(ModemConfigChoice index){

  //Check if we only should use one configuration
  #ifdef MII_RF_FIXED_CONFIG
    const  MiiRF22Modem::ModemConfig cfg = MII_RF_FIXED_CONFIG;
    setModemRegisters(&cfg);
  #else
    if (index > (sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))
        return false;

    MiiRF22Modem::ModemConfig cfg;
    memcpy_P(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(MiiRF22Modem::ModemConfig));
    setModemRegisters(&cfg);
  #endif
    return true;
}

// REVISIT: top bit is in Header Control 2 0x33
void MiiRF22Modem::setPreambleLength(uint8_t nibbles){
    spiWrite(MII_RF_REG_34_PREAMBLE_LENGTH, nibbles);
}

// Caution doesnt set sync word len in Header Control 2 0x33
void MiiRF22Modem::setSyncWords(const uint16_t syncWords){
    spiBurstWrite(MII_RF_REG_36_SYNC_WORD3, (uint8_t*)&syncWords, sizeof(uint16_t));
}

void MiiRF22Modem::clearRxBuf(){
    ATOMIC_BLOCK_START;
    _bufLen = 0;
    _rxBufValid = false;
    ATOMIC_BLOCK_END;
}

bool MiiRF22Modem::internalRecv(uint8_t* buf, uint8_t* len,bool peek){
  if (!hasAvailable()) return false; //We don't want a collision
  bool ret = false;
  if (buf && len) {
    ATOMIC_BLOCK_START;
    if (*len > _bufLen)	*len = _bufLen;
      memcpy(buf, _buf, *len);
      ATOMIC_BLOCK_END;
  }
  if (!peek) clearRxBuf();
  return true;
}

void MiiRF22Modem::clearTxBuf(){
    ATOMIC_BLOCK_START;
    _bufLen = 0;
    _txBufSentIndex = 0;
    ATOMIC_BLOCK_END;
}

void MiiRF22Modem::startTransmit(){
    sendNextFragment(); // Actually the first fragment
    spiWrite(MII_RF_REG_3E_PACKET_LENGTH, _bufLen); // Total length that will be sent
    setModeTx(); // Start the transmitter, turns off the receiver
    _txTime=millis();
}

// Restart the transmission of a packet that had a problem
void MiiRF22Modem::restartTransmit(){
    _mode = MII_RF_MODE_IDLE;
    _txBufSentIndex = 0;
    startTransmit();
}

bool MiiRF22Modem::send(const uint8_t* data, uint8_t len){
   #if MIIRF_SERIAL >= 2
    Serial.print("Send CMD:");Serial.print(_txHeaderId);Serial.print(' ');Serial.print(_txHeaderTo);Serial.print(' ');Serial.println(len);Serial.flush();
   #endif
   bool ret = beforeSend(data,len);
   if (ret) {
    idle((_txTime + _airTime + MII_RF_SEND_DELAY)-millis()); //Make sure we send message in correct time seq

      waitPacketSent();
      ATOMIC_BLOCK_START;
      if (!fillTxBuf(data, len))
	      ret = false;
      else
	     startTransmit();
      ATOMIC_BLOCK_END;
      if (ret) afterSend(data,len);
    }
    return ret;
}

bool MiiRF22Modem::fillTxBuf(const uint8_t* data, uint8_t len){
    clearTxBuf();
    if (!len)
	return false;
    return appendTxBuf(data, len);
}

bool MiiRF22Modem::appendTxBuf(const uint8_t* data, uint8_t len){
    if (((uint16_t)_bufLen + len) > MII_RF_MESSAGE_LEN)
	return false;
    ATOMIC_BLOCK_START;
    memcpy(_buf + _bufLen, data, len);
    _bufLen += len;
    ATOMIC_BLOCK_END;
//    printBuffer("txbuf:", _buf, _bufLen);
    return true;
}

// Assumption: there is currently <= MII_RF_TXFFAEM_THRESHOLD bytes in the Tx FIFO
void MiiRF22Modem::sendNextFragment(){
  if (_txBufSentIndex < _bufLen) {
	  // Some left to send?
	  uint8_t len = _bufLen - _txBufSentIndex;
	  // But dont send too much
	  if (len > (MII_RF_FIFO_SIZE - MII_RF_TXFFAEM_THRESHOLD - 1))
	      len = (MII_RF_FIFO_SIZE - MII_RF_TXFFAEM_THRESHOLD - 1);
	  spiBurstWrite(MII_RF_REG_7F_FIFO_ACCESS, _buf + _txBufSentIndex, len);
   //	printBuffer("frag:", _buf  + _txBufSentIndex, len);
	 _txBufSentIndex += len;
  }
}

// Assumption: there are at least MII_RF_RXFFAFULL_THRESHOLD in the RX FIFO
// That means it should only be called after a RXFFAFULL interrupt
void MiiRF22Modem::readNextFragment(){
    if (((uint16_t)_bufLen + MII_RF_RXFFAFULL_THRESHOLD) > MII_RF_MESSAGE_LEN)
	return; // Hmmm receiver overflow. Should never occur

    // Read the MII_RF_RXFFAFULL_THRESHOLD octets that should be there
    spiBurstRead(MII_RF_REG_7F_FIFO_ACCESS, _buf + _bufLen, MII_RF_RXFFAFULL_THRESHOLD);
    _bufLen += MII_RF_RXFFAFULL_THRESHOLD;
}

// Clear the FIFOs
void MiiRF22Modem::resetFifos(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRRX | MII_RF_FFCLRTX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

// Clear the Rx FIFO
void MiiRF22Modem::resetRxFifo(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRRX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

// CLear the TX FIFO
void MiiRF22Modem::resetTxFifo(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRTX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

void MiiRF22Modem::setHeaderTo(uint8_t to){
    MiiGenericModem::setHeaderTo(to);
    spiWrite(MII_RF_REG_3A_TRANSMIT_HEADER3, _txHeaderTo);
}

void MiiRF22Modem::setHeaderFrom(uint8_t from){
    MiiGenericModem::setHeaderFrom(from);
    spiWrite(MII_RF_REG_3B_TRANSMIT_HEADER2, _txHeaderFrom);
}

void MiiRF22Modem::setHeaderId(uint8_t id){
     MiiGenericModem::setHeaderId(id);
    spiWrite(MII_RF_REG_3C_TRANSMIT_HEADER1, _txHeaderId);
}

void MiiRF22Modem::setHeaderFlags(uint8_t set, uint8_t clear){
    MiiGenericModem::setHeaderFlags(set,clear);
    spiWrite(MII_RF_REG_3D_TRANSMIT_HEADER0, _txHeaderFlags);
}

void MiiRF22Modem::setPromiscuous(bool promiscuous){
    MiiGenericModem::setPromiscuous(promiscuous);
    spiWrite(MII_RF_REG_43_HEADER_ENABLE3, _promiscuous ? 0x00 : 0xff);
}

bool MiiRF22Modem::setCRCPolynomial(CRCPolynomial polynomial){
  if (polynomial >= CRC_CCITT &&	polynomial <= CRC_Biacheva) {
	  _polynomial = polynomial;
	  return true;
  } else return false;
}

void MiiRF22Modem::setAddress(uint8_t address){
  #if MII_MAX_ADDRESS==0
   spiWrite(MII_RF_REG_3F_CHECK_HEADER3, address);
  #endif
  MiiGenericModem::setAddress(address);
}

// Returns true if centre + (fhch * fhs) is within limits
// Caution, different versions of the RF22 support different max freq
// so YMMV
bool MiiRF22Modem::setFrequency(float centre, float width){
  uint8_t fbsel = MII_RF_SBSEL;
  uint8_t afclimiter;
  if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
	  return false;
  if (centre >= 480.0) {
	  if (width < 0.0 || width > 0.318750)
	    return false;
	  centre /= 2;
	  fbsel |= MII_RF_HBSEL;
	  afclimiter = width * 1000000.0 / 1250.0;
  } else {
	  if (width < 0.0 || width > 0.159375)
	    return false;
	  afclimiter = width * 1000000.0 / 625.0;
  }
  centre /= 10.0;
  float integerPart = floor(centre);
  float fractionalPart = centre - integerPart;

  uint8_t fb = (uint8_t)integerPart - 24; // Range 0 to 23
  fbsel |= fb;
  uint16_t fc = fractionalPart * 64000;
  spiWrite(MII_RF_REG_73_FREQUENCY_OFFSET1, 0);  // REVISIT
  spiWrite(MII_RF_REG_74_FREQUENCY_OFFSET2, 0);
  spiWrite(MII_RF_REG_75_FREQUENCY_BAND_SELECT, fbsel);
  spiWrite(MII_RF_REG_76_NOMINAL_CARRIER_FREQUENCY1, fc >> 8);
  spiWrite(MII_RF_REG_77_NOMINAL_CARRIER_FREQUENCY0, fc & 0xff);
  spiWrite(MII_RF_REG_2A_AFC_LIMITER, afclimiter);
  return !(statusRead() & MII_RF_FREQERR);
}

void MiiRF22Modem::setGpioReversed(bool gpioReversed){
    // Ensure the antenna can be switched automatically according to transmit and receive
    // This assumes GPIO0(out) is connected to TX_ANT(in) to enable tx antenna during transmit
    // This assumes GPIO1(out) is connected to RX_ANT(in) to enable rx antenna during receive
 if (gpioReversed)
    {// Reversed for HAB-RFM22B-BOA HAB-RFM22B-BO, also Si4432 sold by Dorji.com via Tindie.com.
    spiWrite(MII_RF_REG_0B_GPIO_CONFIGURATION0, 0x15) ; // RX state
    spiWrite(MII_RF_REG_0C_GPIO_CONFIGURATION1, 0x12) ; // TX state
 } else {
    spiWrite(MII_RF_REG_0B_GPIO_CONFIGURATION0, 0x12) ; // TX state
    spiWrite(MII_RF_REG_0C_GPIO_CONFIGURATION1, 0x15) ; // RX state
 }
}
