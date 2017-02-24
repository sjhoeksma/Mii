#include <MiiRFClass.h>

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

// Interrupt vectors for the 2 Arduino interrupt pins
// Each interrupt can be handled by a different instance of RF22, allowing you to have
// 2 RF22s per Arduino
MiiRFClass* MiiRFClass::_deviceForInterrupt[MII_RF_NUM_INTERRUPTS] = {0, 0, 0};
uint8_t MiiRFClass::_interruptCount = 0; // Index into _deviceForInterrupt for next device

// These are indexed by the values of ModemConfigChoice
// Canned modem configurations generated with
// http://www.hoperf.com/upload/rf/RF22B%2023B%2031B%2042B%2043B%20Register%20Settings_RevB1-v5.xls
// Stored in flash (program) memory to save SRAM
#ifndef MII_RF_FIXED_CONFIG
PROGMEM static const MiiRFClass::ModemConfig MODEM_CONFIG_TABLE[] = {
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

MiiRFClass::MiiRFClass(uint8_t slaveSelectPin, uint8_t interruptPin, uint8_t sdnPin) :
    _slaveSelectPin(slaveSelectPin),
    _interruptPin(interruptPin),
    _sdnPin(sdnPin),
    _timeSync(true)
{
  _idleMode = MII_RF_XTON; // Default idle state is READY mode
  _mode = MII_RF_MODE_IDLE; // We start up in idle mode
  _polynomial = CRC_16_IBM; // Historical
  _airTime=MII_RF_MIN_MSG_INTERVAL;
  _timeout=MII_RF_DEF_TIMEOUT;
  _retryCount=MII_RF_DEFAULT_RETRY;
  _drift=MII_RF_DEFAULT_DRIFT;
 }

bool  MiiRFClass::init(){
 return init(0);
}

/// A more easy init procedure
bool MiiRFClass::init(uint8_t address,uint16_t syncWords,uint8_t channel,uint8_t power,bool isMaster,ModemConfigChoice index){
    _isReady=false;
    power_on_reset();

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
#if F_CPU <= 8000000
    SPI.setClockDivider(SPI_CLOCK_DIV8);
#else
    SPI.setClockDivider(SPI_CLOCK_DIV16);
#endif
    SPI.begin();
     #if SPI_ISOLATE
    _SPCR = SPCR;
    _SPSR = SPSR;
    #endif

    // Initialise the slave select pin
    pinMode(_slaveSelectPin, OUTPUT);
    digitalWrite(_slaveSelectPin, HIGH);
    delay(100); //Give some time

    // Determine the interrupt number that corresponds to the interruptPin
    int interruptNumber = digitalPinToInterrupt(_interruptPin);
    if (interruptNumber == NOT_AN_INTERRUPT) return false;

    // Software reset the device
    reset();

    // Get the device type and check it
    // This also tests whether we are really connected to a device
    _deviceType = spiRead(MII_RF_REG_00_DEVICE_TYPE);
    if (   _deviceType != MII_RF_DEVICE_TYPE_RX_TRX
        && _deviceType != MII_RF_DEVICE_TYPE_TX)
    {
	    return false;
    }

    // Add by Adrien van den Bossche <vandenbo@univ-tlse2.fr> for Teensy
    // ARM M4 requires the below. else pin interrupt doesn't work properly.
    // On all other platforms, its innocuous, belt and braces
    pinMode(_interruptPin, INPUT);

     // Enable interrupts
    spiWrite(MII_RF_REG_05_INTERRUPT_ENABLE1, MII_RF_ENTXFFAEM | MII_RF_ENRXFFAFULL | MII_RF_ENPKSENT | MII_RF_ENPKVALID | MII_RF_ENCRCERROR | MII_RF_ENFFERR);
    spiWrite(MII_RF_REG_06_INTERRUPT_ENABLE2, MII_RF_ENPREAVAL);


    // Set up interrupt handler
    // Since there are a limited number of interrupt glue functions isr*() available,
    // we can only support a limited number of devices simultaneously
    // On some devices, notably most Arduinos, the interrupt pin passed in is actually the
    // interrupt number. You have to figure out the interruptnumber-to-interruptpin mapping
    // yourself based on knowledge of what Arduino board you are running on.
    _deviceForInterrupt[_interruptCount] = this;
    if (_interruptCount == 0)
       attachInterrupt(interruptNumber, isr0, FALLING);
    else if (_interruptCount == 1)
	     attachInterrupt(interruptNumber, isr1, FALLING);
    else if (_interruptCount == 2)
	     attachInterrupt(interruptNumber, isr2, FALLING);
    else
	    return false; // Too many devices, not enough interrupt vectors
    _interruptCount++;

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

/*
    setAddress(address);
    setSyncWords(syncWords);
    #ifdef MII_RF_FREQ_FIXED
      setModemConfig(index);
      setFrequency(MII_RF_FREQ_FIXED, MII_RF_CHANNEL_WIDTH);
      setTxPower(power);
    #else
     setChannel(channel,power,index);
    #endif
*/
    setAddress(address);
    setSyncWords(syncWords);
    setModemConfig(index);
    #ifdef MII_RF_FREQ_FIXED
      setFrequency(MII_RF_FREQ_FIXED, MII_RF_CHANNEL_WIDTH);
    #else
     setChannel(channel);
    #endif
    setTxPower(power);

    if (isMaster) setMaster(address);
    _isReady=true;
    return true;
}

void MiiRFClass::chipSelectHigh(void){
  digitalWrite(_slaveSelectPin, HIGH);
  #if SPI_ISOLATE
   SPDR = 0xFF;
   while (!(SPSR & (1 << SPIF)));
   SPCR=_SPCRO;
   SPSR=_SPSRO;
  #endif
}

void MiiRFClass::chipSelectLow(void){
 #if SPI_ISOLATE
  _SPCRO=SPCR;
  _SPSRO=SPSR;
  SPCR = _SPCR;
  SPSR = _SPSR;
 #endif
 digitalWrite(_slaveSelectPin, LOW);
}


// C++ level interrupt handler for this instance
void MiiRFClass::handleInterrupt(){
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

    }
    if (_lastInterruptFlags[0] & MII_RF_ICRCERROR) {
      clearRxBuf();
	    resetRxFifo();
	    _mode = MII_RF_MODE_IDLE;
	    setModeRx(); // Keep trying
    }
    if (_lastInterruptFlags[1] & MII_RF_IPREAVAL) {
	    _lastRssi = (int8_t)(-120 + ((spiRead(MII_RF_REG_26_RSSI) / 2)));
	    resetRxFifo();
	    clearRxBuf();
    }
}

// These are low level functions that call the interrupt handler for the correct
// instance of RF22.
// 2 interrupts allows us to have 2 different devices
void MiiRFClass::isr0() {
    if (_deviceForInterrupt[0])
	_deviceForInterrupt[0]->handleInterrupt();
}

void MiiRFClass::isr1() {
    if (_deviceForInterrupt[1])
	_deviceForInterrupt[1]->handleInterrupt();
}
void MiiRFClass::isr2() {
    if (_deviceForInterrupt[2])
	_deviceForInterrupt[2]->handleInterrupt();
}

void MiiRFClass::reset(){
    spiWrite(MII_RF_REG_07_OPERATING_MODE1, MII_RF_SWRES);
    // Wait for it to settle
    delay(1); // SWReset time is nominally 100usec
}

uint8_t MiiRFClass::spiRead(uint8_t reg){
    uint8_t val;

    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg & ~MII_RF_SPI_WRITE_MASK); // Send the address with the write mask off
    val = SPI.transfer(0); // The written value is ignored, reg value is read
    chipSelectHigh();
    ATOMIC_BLOCK_END;
    return val;
}

void MiiRFClass::spiWrite(uint8_t reg, uint8_t val){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg | MII_RF_SPI_WRITE_MASK); // Send the address with the write mask on
    SPI.transfer(val); // New value follows
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}

void MiiRFClass::spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg & ~MII_RF_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
      	*dest++ = SPI.transfer(0);
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}

void MiiRFClass::spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len){
    ATOMIC_BLOCK_START;
    chipSelectLow();
    SPI.transfer(reg | MII_RF_SPI_WRITE_MASK); // Send the start address with the write mask on
    while (len--)
      	SPI.transfer(*src++);
    chipSelectHigh();
    ATOMIC_BLOCK_END;
}

uint8_t MiiRFClass::statusRead(){
    return spiRead(MII_RF_REG_02_DEVICE_STATUS);
}

uint8_t MiiRFClass::adcRead(uint8_t adcsel,
                      uint8_t adcref ,
                      uint8_t adcgain,
                      uint8_t adcoffs)
{
    uint8_t configuration = adcsel | adcref | (adcgain & MII_RF_ADCGAIN);
    spiWrite(MII_RF_REG_0F_ADC_CONFIGURATION, configuration | MII_RF_ADCSTART);
    spiWrite(MII_RF_REG_10_ADC_SENSOR_AMP_OFFSET, adcoffs);

    // Conversion time is nominally 305usec
    // Wait for the DONE bit
    while (!(spiRead(MII_RF_REG_0F_ADC_CONFIGURATION) & MII_RF_ADCDONE))
	;
    // Return the value
    return spiRead(MII_RF_REG_11_ADC_VALUE);
}

uint8_t MiiRFClass::temperatureRead(uint8_t tsrange, uint8_t tvoffs){
    spiWrite(MII_RF_REG_12_TEMPERATURE_SENSOR_CALIBRATION, tsrange | MII_RF_ENTSOFFS);
    spiWrite(MII_RF_REG_13_TEMPERATURE_VALUE_OFFSET, tvoffs);
    return adcRead(MII_RF_ADCSEL_INTERNAL_TEMPERATURE_SENSOR | MII_RF_ADCREF_BANDGAP_VOLTAGE);
}

uint16_t MiiRFClass::wutRead(){
    uint8_t buf[2];
    spiBurstRead(MII_RF_REG_17_WAKEUP_TIMER_VALUE1, buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1]; // Dont rely on byte order
}

// RFM-22 doc appears to be wrong: WUT for wtm = 10000, r, = 0, d = 0 is about 1 sec
void MiiRFClass::setWutPeriod(uint16_t wtm, uint8_t wtr, uint8_t wtd){
    uint8_t period[3];

    period[0] = ((wtr & 0xf) << 2) | (wtd & 0x3);
    period[1] = wtm >> 8;
    period[2] = wtm & 0xff;
    spiBurstWrite(MII_RF_REG_14_WAKEUP_TIMER_PERIOD1, period, sizeof(period));
}

// Returns true if centre + (fhch * fhs) is within limits
// Caution, different versions of the RF22 support different max freq
// so YMMV
bool MiiRFClass::setFrequency(float centre, float afcPullInRange){
    uint8_t fbsel = MII_RF_SBSEL;
    uint8_t afclimiter;
    if (centre < 240.0 || centre > 960.0) // 930.0 for early silicon
	return false;
    if (centre >= 480.0)
    {
	if (afcPullInRange < 0.0 || afcPullInRange > 0.318750)
	    return false;
	centre /= 2;
	fbsel |= MII_RF_HBSEL;
	afclimiter = afcPullInRange * 1000000.0 / 1250.0;
    }
    else
    {
	if (afcPullInRange < 0.0 || afcPullInRange > 0.159375)
	    return false;
	afclimiter = afcPullInRange * 1000000.0 / 625.0;
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

// Step size in 10kHz increments
// Returns true if centre + (fhch * fhs) is within limits
bool MiiRFClass::setFHStepSize(uint8_t fhs){
    spiWrite(MII_RF_REG_7A_FREQUENCY_HOPPING_STEP_SIZE, fhs);
    return !(statusRead() & MII_RF_FREQERR);
}

// Adds fhch * fhs to centre frequency
// Returns true if centre + (fhch * fhs) is within limits
bool MiiRFClass::setFHChannel(uint8_t fhch){
    spiWrite(MII_RF_REG_79_FREQUENCY_HOPPING_CHANNEL_SELECT, fhch);
    return !(statusRead() & MII_RF_FREQERR);
}

uint8_t MiiRFClass::rssiRead(){
    return spiRead(MII_RF_REG_26_RSSI);
}

uint8_t MiiRFClass::ezmacStatusRead(){
    return spiRead(MII_RF_REG_31_EZMAC_STATUS);
}

void MiiRFClass::setMode(uint8_t mode){
    spiWrite(MII_RF_REG_07_OPERATING_MODE1, mode);
}

void MiiRFClass::setModeIdle(){
    if (_mode != MII_RF_MODE_IDLE)
    {
	setMode(_idleMode);
	_mode = MII_RF_MODE_IDLE;
    }
}

void MiiRFClass::setModeRx(){
    if (_mode != MII_RF_MODE_RX)
    {
	setMode(_idleMode | MII_RF_RXON);
	_mode = MII_RF_MODE_RX;
    }
}

void MiiRFClass::setModeTx(){
    if (_mode != MII_RF_MODE_TX)
    {
	setMode(_idleMode | MII_RF_TXON);
	_mode = MII_RF_MODE_TX;
	// Hmmm, if you dont clear the RX FIFO here, then it appears that going
	// to transmit mode in the middle of a receive can corrupt the
	// RX FIFO
	resetRxFifo();
    }
}

uint8_t  MiiRFClass::mode(){
    return _mode;
}

void MiiRFClass::setTxPower(uint8_t power){
 //  #if MII_VERSION >=40 && MII_VERSION <50
 //    spiWrite(MII_RF_REG_6D_TX_POWER, power | MII_RF_TXPOW_LNA_SW);
 //  #else
    spiWrite(MII_RF_REG_6D_TX_POWER, power);
 //  #endif
}

// Sets registers from a canned modem configuration structure
void MiiRFClass::setModemRegisters(const ModemConfig* config){
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
bool MiiRFClass::setModemConfig(ModemConfigChoice index){

  //Check if we only should use one configuration
  #ifdef MII_RF_FIXED_CONFIG
    const  MiiRFClass::ModemConfig cfg = MII_RF_FIXED_CONFIG;
    setModemRegisters(&cfg);
  #else
    if (index > (sizeof(MODEM_CONFIG_TABLE) / sizeof(ModemConfig)))
        return false;

    MiiRFClass::ModemConfig cfg;
    memcpy_P(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(MiiRFClass::ModemConfig));
    setModemRegisters(&cfg);
  #endif
    return true;
}

// REVISIT: top bit is in Header Control 2 0x33
void MiiRFClass::setPreambleLength(uint8_t nibbles){
    spiWrite(MII_RF_REG_34_PREAMBLE_LENGTH, nibbles);
}

// Caution doesnt set sync word len in Header Control 2 0x33
void MiiRFClass::setSyncWords(const uint8_t* syncWords, uint8_t len){
    spiBurstWrite(MII_RF_REG_36_SYNC_WORD3, syncWords, len);
}

void MiiRFClass::clearRxBuf(){
    ATOMIC_BLOCK_START;
    _bufLen = 0;
    _rxBufValid = false;
    ATOMIC_BLOCK_END;
}

// Blocks until a valid message is received
void MiiRFClass::waitAvailable(){
    while (!available(false)) {
      idle();
	  };
}

// Blocks until a valid message is received or timeout expires
// Return true if there is a message available
// Works correctly even on millis() rollover
bool MiiRFClass::waitAvailableTimeout(uint16_t timeout){
    unsigned long starttime = millis();
    while ((millis() - starttime) < timeout) {
        if (available(false))
           return true;
        idle();
    }
    return false;
}

void MiiRFClass::waitPacketSent(){
    while (_mode == MII_RF_MODE_TX){idle();}; // Wait for any previous transmit to finish
}

bool MiiRFClass::waitPacketSent(uint16_t timeout)
{
    unsigned long starttime = millis();
    while ((millis() - starttime) < timeout) {
        if (_mode != MII_RF_MODE_TX) // Any previous transmit finished?
           return true;
        idle();
    }
    return false;
}

//Wrapper for the old recieved
bool MiiRFClass::recv(uint8_t* buf, uint8_t* len,bool peek){
 return recv(true,buf,len,peek);
}

 //Discard ad message by receving the message without ack
bool MiiRFClass::discard(bool allowCollision){
 bool ret = false;
  #if MII_MAX_MSG_CACHE
   if (allowCollision && _msgCount) ret=popMsg();
  #endif
  if (!ret) { //We had no collision to process, see if there was normal data
    if (!hasAvailable()) return false; //We don't want any processing
    clearRxBuf();
    _rxHeaderTo=0;
    ret = true;
  }
  return ret;
}

bool MiiRFClass::recv(bool allowCollision,uint8_t* buf, uint8_t* len,bool peek){
  bool ret = false;
  #if MII_MAX_MSG_CACHE
   if (allowCollision && _msgCount) ret=popMsg(buf,len,peek);
  #endif
  if (!ret) { //We had no collision to process, see if there is other data
    if (!available(false)) return false; //We don't want a collision

    if (buf && len) {
      ATOMIC_BLOCK_START;
      if (*len > _bufLen)	*len = _bufLen;
      memcpy(buf, _buf, *len);
      ATOMIC_BLOCK_END;
    }
    if (!peek) clearRxBuf();
    ret = true;
  }
  if (ret && !peek) {
     if (_rxHeaderFrom!=MII_BROADCAST_ADDRESS && _rxHeaderTo==_thisAddress &&
         (_rxHeaderFlags & MII_RF_FLAGS_ACK) && _rxTime!=_ackTime) {
       #if MIIRF_SERIAL
       Serial.print("Sending Ack:");Serial.print(_rxHeaderId);Serial.print(' ');Serial.println(_rxHeaderFrom);Serial.flush();
       #endif
       _ackTime=_rxTime;
       cbi(_rxHeaderFlags,MII_RF_FLAGS_ACK); //Clear Ack Flag
       setHeaderId(MII_C_ACK);
       setHeaderFlags(0);
       setHeaderTo(_rxHeaderFrom);
       send((uint8_t*)&_rxHeaderId, sizeof(_rxHeaderId));
       waitPacketSent();
    }
  }
  return ret;
}

void MiiRFClass::clearTxBuf(){
    ATOMIC_BLOCK_START;
    _bufLen = 0;
    _txBufSentIndex = 0;
    ATOMIC_BLOCK_END;
}

void MiiRFClass::startTransmit(){
    sendNextFragment(); // Actually the first fragment
    spiWrite(MII_RF_REG_3E_PACKET_LENGTH, _bufLen); // Total length that will be sent
    setModeTx(); // Start the transmitter, turns off the receiver
    _txTime=millis();
}

// Restart the transmission of a packet that had a problem
void MiiRFClass::restartTransmit(){
    _mode = MII_RF_MODE_IDLE;
    _txBufSentIndex = 0;
//	    Serial.println("Restart");
    startTransmit();
}

bool MiiRFClass::send(const uint8_t* data, uint8_t len){
    bool ret = beforeSend(data,len);
    idle((_txTime + _airTime + MII_RF_SEND_DELAY)-millis()); //Make sure we send message in correct time seq
    waitPacketSent();
    ATOMIC_BLOCK_START;
    if (!fillTxBuf(data, len))
	    ret = false;
    else
	    startTransmit();
    ATOMIC_BLOCK_END;
    if (ret) afterSend(data,len);
//    printBuffer("send:", data, len);
    return ret;
}

bool MiiRFClass::fillTxBuf(const uint8_t* data, uint8_t len){
    clearTxBuf();
    if (!len)
	return false;
    return appendTxBuf(data, len);
}

bool MiiRFClass::appendTxBuf(const uint8_t* data, uint8_t len){
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
void MiiRFClass::sendNextFragment(){
    if (_txBufSentIndex < _bufLen)
    {
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
void MiiRFClass::readNextFragment(){
    if (((uint16_t)_bufLen + MII_RF_RXFFAFULL_THRESHOLD) > MII_RF_MESSAGE_LEN)
	return; // Hmmm receiver overflow. Should never occur

    // Read the MII_RF_RXFFAFULL_THRESHOLD octets that should be there
    spiBurstRead(MII_RF_REG_7F_FIFO_ACCESS, _buf + _bufLen, MII_RF_RXFFAFULL_THRESHOLD);
    _bufLen += MII_RF_RXFFAFULL_THRESHOLD;
}

// Clear the FIFOs
void MiiRFClass::resetFifos(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRRX | MII_RF_FFCLRTX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

// Clear the Rx FIFO
void MiiRFClass::resetRxFifo(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRRX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

// CLear the TX FIFO
void MiiRFClass::resetTxFifo(){
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, MII_RF_FFCLRTX);
    spiWrite(MII_RF_REG_08_OPERATING_MODE2, 0);
}

// Default implmentation does nothing. Override if you wish
void MiiRFClass::handleExternalInterrupt(){
}

// Default implmentation does nothing. Override if you wish
void MiiRFClass::handleWakeupTimerInterrupt(){
}

void MiiRFClass::setHeaderTo(uint8_t to){
    _txHeaderTo=to;
    spiWrite(MII_RF_REG_3A_TRANSMIT_HEADER3, to);
}

void MiiRFClass::setHeaderFrom(uint8_t from){
    _txHeaderFrom=from;
    spiWrite(MII_RF_REG_3B_TRANSMIT_HEADER2, from);
}

void MiiRFClass::setHeaderId(uint8_t id){
    _txHeaderId=id;
    spiWrite(MII_RF_REG_3C_TRANSMIT_HEADER1, id);
}

void MiiRFClass::setHeaderFlags(uint8_t flags){
    _txHeaderFlags=flags;
    spiWrite(MII_RF_REG_3D_TRANSMIT_HEADER0, flags);
}

uint8_t MiiRFClass::headerTo(){
    return _rxHeaderTo;
}

uint8_t MiiRFClass::headerFrom(){
    return _rxHeaderFrom;
}

uint8_t MiiRFClass::headerId(){
    return _rxHeaderId;
}

uint8_t MiiRFClass::headerFlags(){
    return _rxHeaderFlags;
}

uint8_t MiiRFClass::lastRssi(){
    return _lastRssi;
}

void MiiRFClass::setPromiscuous(bool promiscuous){
    _isPromiscuous=promiscuous;
    spiWrite(MII_RF_REG_43_HEADER_ENABLE3, promiscuous ? 0x00 : 0xff);
}

bool MiiRFClass::setCRCPolynomial(CRCPolynomial polynomial)
{
    if (polynomial >= CRC_CCITT &&
	polynomial <= CRC_Biacheva)
    {
	_polynomial = polynomial;
	return true;
    }
    else
	return false;
}

void MiiRFClass::power_on_reset(){
    if (_sdnPin!=0xff) {
      // Sigh: its necessary to control the SDN pin to reset this ship.
      // Tying it to GND does not produce reliable startups
      // Per Si4464 Data Sheet 3.3.2
      pinMode(_sdnPin, OUTPUT);
      digitalWrite(_sdnPin, HIGH);
      delay(25);
      digitalWrite(_sdnPin, LOW);
    }
    delay(50);
    #if MII_MAX_MSG_CACHE
    _msgCount=0;
    #endif
}

//Just wait idle
void MiiRFClass::idle(long time) {
  if (time<=0){
    YIELD;
    return;
  }
  uint32_t t=time+millis();
  do {
   YIELD;
  } while (t>millis());
}

#if MII_MAX_MSG_CACHE
bool MiiRFClass::pushMsg(){
 bool ret = false;
 if (available(false) && _msgCount<MII_MAX_MSG_CACHE) {
    #if MIIRF_SERIAL
     Serial.println("COL PUSH: DATA");
    #endif
    _msg[_msgCount].headerId=_rxHeaderId;
    _msg[_msgCount].headerFrom=_rxHeaderFrom;
    _msg[_msgCount].headerTo=_rxHeaderTo;
    _msg[_msgCount].headerFlags=(_rxHeaderFlags & ~_BV(MII_RF_FLAGS_ACK)); //We will ack below
    _msg[_msgCount].length=MII_RF_MESSAGE_LEN;
    _msg[_msgCount].rxTime=_rxTime;
    if (recv(false,_msg[_msgCount].data,&_msg[_msgCount].length)) { //We will allow ack to be send
        _msgCount++; //We have read a new collision
        ret = true;
    }
  }
  if (_msgCount) {
    //Fill the headers with the data available
    _rxHeaderId=_msg[0].headerId;
    _rxHeaderFrom=_msg[0].headerFrom;
    _rxHeaderTo=_msg[0].headerTo;
    _rxHeaderFlags=_msg[0].headerFlags;
    _rxTime=_msg[0].rxTime;
  }
  return ret;
}

bool MiiRFClass::popMsg(uint8_t* buf, uint8_t* len,bool peek){
  if (_msgCount) {
     if (buf && len){
       *len=min(*len,_msg[0].length);
       memmove(buf,_msg[0].data,*len);
      };
      if (peek) return true;
     //Check if we are in receiving mode and have already data in.
     //If so we have to move it in and out
      uint8_t rxHeaderId=_msg[0].headerId;
      uint8_t rxHeaderFrom=_msg[0].headerFrom;
      uint8_t rxHeaderTo=_msg[0].headerTo;
      uint8_t rxHeaderFlags=_msg[0].headerFlags;
      uint32_t rxTime=_msg[0].rxTime;

      _msgCount--;
      memmove(&_msg[0],&_msg[1],sizeof(msgRec_t)*_msgCount);
      if (available(false)) {
        #if MIIRF_SERIAL
          Serial.println(F("COL POP: WITH DATA IN"));
        #endif
        //We have temporary save the collision header before swap record
        pushMsg(); //Add the available record to the stack

        } else { //Free up the collision
         #if MIIRF_SERIAL
          Serial.println(F("COL POP: WITHOUT DATA IN"));
        #endif
       }
    //We save record in collision so now we can swap headers
      _rxHeaderId=rxHeaderId;
      _rxHeaderFrom=rxHeaderFrom;
      _rxHeaderTo=rxHeaderTo;
      _rxHeaderFlags=rxHeaderFlags;
      _rxTime=rxTime;
      return true;
  }
  return false;
}
#endif

bool MiiRFClass::recv8_t(uint8_t  &buf,bool allowCollision){uint8_t len=sizeof(uint8_t);return recv(allowCollision,(uint8_t*)&buf,&len);}
bool MiiRFClass::recv16_t(uint16_t  &buf,bool allowCollision){uint8_t len=sizeof(uint16_t);return recv(allowCollision,(uint8_t*)&buf,&len);}
bool MiiRFClass::recv32_t(uint32_t  &buf,bool allowCollision){uint8_t len=sizeof(uint32_t);return recv(allowCollision,(uint8_t*)&buf,&len);}

bool MiiRFClass::syncTime(uint8_t address){
   if (isMaster()) return false;
//   _timeDiff=0; //Block sending data
   //Send a request for timesync to the MASTER, on receving the request Master will only accept messages from this device
   //Master will send his Time T[0] to this device will receive at T[1] and we will response with new request, skipped because of start master
   //Master will send his Time T[2] to this device will receive at T[3] and we will response with new request
   //Master will send his Time T[4] to this device will receive at T[5] and start calculation
   uint8_t _try=0;
   uint32_t T[6];
   uint8_t toAddress=MII_BROADCAST_ADDRESS;
   for (uint8_t i=0;i<6;){
     if (!(sendCmd(MII_C_TIMESYNC,address) &&
       waitForCmd(MII_C_TIMESYNC,(uint8_t*)&T[i],sizeof(uint32_t),address)))
     { // Allow search for other time sync
      _try++;
      if (_try<4 && i==0) continue;
      #if MIIRF_SERIAL >= 2
          Serial.println("Failed sync");Serial.flush();
      #endif
      _lastTime+=1000; //1 Seconds retry
      return false;
     }
    // Serial.print("T[");Serial.print(i);Serial.print("]=");Serial.print(T[i]);Serial.print(" T[");Serial.print(i+1);Serial.print("]=");Serial.println(_rxTime);Serial.flush();
     i++; //The send time
     if (address==MII_BROADCAST_ADDRESS) address=_rxHeaderFrom; //toAddress can be broadcast for next rounds we take the real address
     T[i++]=_rxTime; //The receive time
   } //For
   _masterAddress=address; //Valid time so set the masterAddress
   //Calculate transmit time by dividing round trip using the two times (T[2] and T[4]) of master by 2
   //Add this round trip to the T[4]send time of master and substract the receive time T[5]
   _airTime=(T[4]-T[2])/2;
   _timeDiff=T[4]+_airTime-T[5];
   _timeout=_airTime*2+MII_RF_MIN_MSG_INTERVAL*2;
   timeChanged();
   #if MIIRF_SERIAL >= 2
     Serial.print("New Diff:(");Serial.print(_airTime);Serial.print(')');Serial.println(_timeDiff);Serial.flush();
   #endif
   _lastTime=millis();
   return true;
}

void MiiRFClass::changeChannel(uint8_t channel) {
  if (isMaster()) {
    //Master will broadcast SET_CHANNEL for 10 times, 1 second intervall to all listening
    for (int i=0;i<10;i++) {
      sendCmd(MII_C_CHANNEL,&channel,sizeof(uint8_t));
      delay(1000);
    }
  }
  setChannel(channel);
}

bool MiiRFClass::internalProcess(){
 bool ret = false;
 if (_inAvailable || available()) {
   #if MIIRF_SERIAL >= 1
     Serial.print("New:");Serial.print(_rxHeaderFrom);Serial.print(' ');Serial.println(_rxHeaderId);Serial.flush();
   #endif
   beforeProcess();
   #if MII_MAX_ADDRESS
   //We have data build up the addressTable
   uint8_t a=0;
   for (a=0;a<_addressCount && _address[a].id!=_rxHeaderFrom;a++){}
   if (a<_addressCount) { //We found the record, shift first records to freeup record
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*a);
   } else { //New Record, insert at first place
     memmove(&_address[1],&_address[0],sizeof(addressRec_t)*_addressCount);
     if (_addressCount<MII_MAX_ADDRESS-1) _addressCount++;
   }
   //Fill the first record with data for recieved address
   _address[0].id=_rxHeaderFrom;
   _address[0].seen=millis(); //Create seconds
   _address[0].rssi=lastRssi();

	 //Filter the messages, to see if they are for us
	 if (_rxHeaderFrom == _thisAddress ||
	    !(_rxHeaderTo == _thisAddress || _rxHeaderTo == MII_BROADCAST_ADDRESS)
	    ) { //Discard the message, it is not for us, no ack
	    discard();
	    return false;
	 }
	 #endif

   switch (_rxHeaderId){
    case MII_C_TIMESYNC :
       recv(); //Clear TimeSync CMD
       if ( isMaster() || _rxHeaderTo==_thisAddress) { //Only master or direct requests will reply on Time Sync commands
         //Master will send 3 times data, no retry and will not wait for last reqst
         uint32_t t;
         for (uint8_t i=0;i<3;i++) {
           t=time();
           if (!sendCmd(MII_C_TIMESYNC,(uint8_t*)&t,sizeof(uint32_t),_rxHeaderFrom)) break;
           if (i==2 || !waitForCmd(MII_C_TIMESYNC,0,0,_rxHeaderFrom)) break; //Exit if did not send or got correct response
         }
         //As Master we will send a time command close .7Sec after we did do a sync
         if (isMaster() && t>_timeInterval) _lastTime=t+700-_timeInterval;
       } else {
          //When somebody is doing time sync make sure we don't send a message
          _txTime=millis()+_airTime*2;
       }
       break;

   case MII_C_REQ_TIME:
      recv(); //Clear Request Time CMD
      if (isMaster()) sendTime();
      break;

   #if MII_SUPPORT_CHANNEL_SET
   case MII_C_CHANNEL: {
         uint8_t channel;
         if (recv8_t(channel)) changeChannel(channel);
    } break;
    #endif

    case MII_C_TIME:
        if (!isMaster()) { //We are just a client and should process this time from master
           uint32_t _time;
           uint8_t _len=sizeof(_time);
           if (recv((uint8_t *)&_time,&_len) && _time) { //Only accept time if it is set
             long diff = time(_rxTime)-_airTime-_time;
             _lastTime=_rxTime;
             #if MIIRF_SERIAL >= 2
              Serial.print("Time:");Serial.print(time(_rxTime));Serial.print("-");Serial.print(time(_airTime));Serial.print("-");Serial.print(_time);Serial.print("=");Serial.print(diff);Serial.print('<');Serial.print(_drift);Serial.print(" [");Serial.print(millis());Serial.print('-');Serial.print(_rxTime);Serial.println("]");Serial.flush();
             #endif
              if (!getTimeSync()) {
               _masterAddress=_rxHeaderFrom; //Valid time so set the masterAddress
               _timeDiff-=diff;
               _lastTime=millis();
               if (abs(diff)>_drift) timeChanged();
              //We balance the time drift by adding half of the drift if difference is within 10ms
              } else if (abs(diff)<=_drift && _timeDiff) { //We need to valid timediff before we calculate diff
                 _timeDiff-=((diff*3)/4); //For bigger drifts gap is closed better (.75)
                 _lastTime=millis();
              } else { //When difference to large request timing
                syncTime();
              }
           }
        } else {
          discard(); //Clear Time CMD, without ack
        }
        break;
    default : ret=true; break; //We have not internal processed so it is available
    }
  } else { //Now data was available to process
    //  check if we should do time sync, when never registered do it fast
    if (_timeInterval && !isMaster() && (_lastTime+(_timeDiff==0 ? _timeInterval/3 : _timeInterval))<millis()){
       #if MIIRF_SERIAL >= 2
         Serial.print("Request NEW TIME:");Serial.print(_timeInterval);Serial.print('+');Serial.print(_lastTime);Serial.print('<');Serial.println(millis());
       #endif
     if (!getTimeSync()) {
        if (_timeDiff==0) sendCmd(MII_C_REQ_TIME);
        _lastTime=millis();
      } else
       syncTime();
    }
  }
  return ret;
}

bool MiiRFClass::hasAvailable(){
 if (!_rxBufValid) {
   if (_mode==MII_RF_MODE_TX) return false;
	 setModeRx(); // Make sure we are receiving
 }
 return _rxBufValid;
}

bool MiiRFClass::available(bool allowCollision){
  //If we allow collision the be retrieved, check if we have them available
   #if MII_MAX_MSG_CACHE
  if (allowCollision && _msgCount) {
     pushMsg(); //If there is real data just pusch it on the stack
     #if MIIRF_SERIAL
      Serial.println(F("COL AV: DATA"));
     #endif
     return true;
  }
  #endif

  //If where are in the internal processing loops return the original availability
  if (_inAvailable) return hasAvailable();

  _inAvailable=true;
  bool ret = internalProcess();
  //Unlock the available loop
  _inAvailable=false;
  if (!ret) idle();
  return ret;
}

//Send a command and wait for Ack when trys is set
bool  MiiRFClass::sendAckCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t trys,uint8_t flags){
  bool ret=false;
  if (trys==0xFF) trys = _retryCount; //trys will be on more than number of retries
  if (trys) trys++; //We always loop so 1 try is always needed
  if (!address) address=_masterAddress; //When no address is specified we user master
  if (!address) return false; //We will not send data to a undefined address
  #if MIIRF_SERIAL >= 2
  Serial.print("Send CMD:");Serial.print(cmd);Serial.print(' ');Serial.print(trys);Serial.print(' ');Serial.print(address);Serial.print(' ');Serial.print(flags);Serial.print(' ');Serial.println(len);Serial.flush();
  #endif
do  {
	setHeaderFlags(trys && (address!=MII_BROADCAST_ADDRESS) ? (MII_RF_FLAGS_ACK | flags) : flags);
  setHeaderId(cmd);
  setHeaderTo(address);
  //When len is 0 we only send cmd as data
  ret = len> 0 ? send(buf, len) : send(&cmd,sizeof(cmd));
  #if MIIRF_SERIAL >= 2
    Serial.print("Send len: ");Serial.println(len);Serial.flush();
  #endif
  waitPacketSent();
 #if MIIRF_SERIAL >= 2
     Serial.println("Send-Done");Serial.flush();
   #endif
  if (ret && trys && address!=MII_BROADCAST_ADDRESS) {
     //Wait for ACK
     ret = waitForCmd(MII_C_ACK,0,0,address);
     //reduce the retry counts
     trys--;
     #if MIIRF_SERIAL
     if (!ret && trys>0) Serial.println(F("RETRY"));
     #endif
  }
} while (!ret && trys>0);

    //Check for master and if we should send update of time, not during sync
  if (_timeInterval && isMaster() && millis()>_lastTime+_timeInterval && cmd!=MII_C_TIMESYNC){
   // Serial.println("Sending master time");
   sendTime();
  }
  //Return to listening
  setModeRx();
  return ret;
}

void MiiRFClass::sendTime() {
  if (isMaster()) {
    setHeaderFlags(0);
    setHeaderId(MII_C_TIME);
    setHeaderTo(MII_BROADCAST_ADDRESS);
    uint32_t _time = time();
    if (send((uint8_t*)&_time,sizeof(_time))) {
       _lastTime=millis();
       waitPacketSent();
    }
  }
}

bool MiiRFClass::sendAckCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,trys,flags);
}

bool  MiiRFClass::sendAckCmd(uint8_t cmd,uint8_t address,uint8_t trys,uint8_t flags){
 return sendAckCmd(cmd,NULL,0,address,trys,flags);
}

//Send a command to a specific address
bool MiiRFClass::sendCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint8_t flags){
 return sendAckCmd(cmd,buf,len,address,0,flags);
}

bool MiiRFClass::sendCmd(uint8_t cmd,uint32_t buf,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,(uint8_t *)&buf,sizeof(uint32_t),address,0,flags);
}

bool MiiRFClass::sendCmd(uint8_t cmd,uint8_t address,uint8_t flags){
  return sendAckCmd(cmd,NULL,0,address,0,flags);
}

bool MiiRFClass::waitForCmd(uint8_t cmd,uint8_t* buf, uint8_t len, uint8_t address,uint16_t timeout){
  if (!timeout) timeout=_timeout;
  if (!address) address=_masterAddress;  //We use master address for ok when not set
  uint32_t failtime=timeout+millis();
  //We wil not acknowledge messages
  while (failtime>millis()){
    if (available(false)) {
       if (_rxHeaderId==cmd && (address==MII_BROADCAST_ADDRESS || address==_rxHeaderFrom)) {
          #if MIIRF_SERIAL
           Serial.print(F("Wait Found:"));Serial.println(cmd);
          #endif
          return recv(false,buf,&len); //We are allowed to send ack we have received it
       }
      #if MIIRF_SERIAL
           Serial.print(F("Wait got:"));Serial.println(cmd);
       #endif
      //We have a collision, received message we did not expect
      //Store it in collision stack if it is not full
      #if MII_MAX_MSG_CACHE
      if (!pushMsg())  //No more room in collision stack, Clear message directly
      #endif
      discard(false);
    }
    idle();
  }
  return false;
}

//Wait for a empty command
bool MiiRFClass::waitForCmd(uint8_t cmd,uint8_t address,uint16_t timeout){
  return waitForCmd(cmd,0,0,address,timeout);
}


uint32_t MiiRFClass::time(uint32_t _time){
 return isMaster() ? (_time ? _time : millis()) : (_timeDiff ? (_time ? _time : millis())+ _timeDiff : _time );
}

bool MiiRFClass::isMaster(){
  return _masterAddress==_thisAddress;
}

void MiiRFClass::setSyncWords(const uint16_t syncWords){
 setSyncWords((const uint8_t*)&syncWords, 2);
}

void MiiRFClass::setChannel(uint8_t channel){
 //Convert kHz to MHz
  float freq= MII_RF_CHANNEL_BASE  + channel*(MII_RF_CHANNEL_RANGE/MII_RF_CHANNEL_COUNT);
  setFrequency(freq, MII_RF_CHANNEL_WIDTH);

}

void MiiRFClass::setChannel(uint8_t channel,uint8_t power,ModemConfigChoice index){
  //setModemConfig(FSK_Rb2_4Fd36);
  setModemConfig(index);

  setChannel(channel);
  /** Code from  https://lowpowerlab.com/forum/index.php?topic=357.0 */
 // spiWrite(0x1D, 0x50);  // Reduce AFC gain (Default is 0x44).
 // spiWrite(0x1E, 0x1A);  // Increase wait time between AFC corrections (Default 0x0A).
 // setPreambleLength(16);  // Increase Preamble to 64 bits (16 x 4bits).

  setTxPower(power);
  //setTxPower(MII_TXPOW_17DBM);
}

void MiiRFClass::setAddress(uint8_t address){
  _thisAddress=address;
  setHeaderFrom(address);
  #if MII_MAX_ADDRESS
  setPromiscuous(true); //We will filter the address ourself in available
  #else
   spiWrite(MII_RF_REG_3F_CHECK_HEADER3, address);
  #endif
  if (!_timeInterval)
    _timeInterval=_timeSync ? MII_TIME_INTERVAL_CLIENT : MII_TIME_INTERVAL_NOTIMESYNC;
}

void MiiRFClass::setMaster(uint8_t address){
  _masterAddress=address;
  if (address==_thisAddress) {
    setTimeSync(true);
    _timeInterval= MII_TIME_INTERVAL_MASTER;
  }
}

void MiiRFClass::setGpioReversed(bool gpioReversed)
{
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

#if MII_MAX_ADDRESS
uint8_t MiiRFClass::nextAddress(uint8_t address,uint32_t liveSpan){
  if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list and our own address
  uint8_t ret=_thisAddress>address ? _thisAddress :0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id>address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiRFClass::prevAddress(uint8_t address,uint32_t liveSpan){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : max(ret,_address[i].id));
  }
  return ret;
}

uint8_t MiiRFClass::firstAddress(uint8_t address,uint32_t liveSpan){
 if (!address) address==_thisAddress; //Set address to internal address if no address set
  //Go through the list
  uint8_t ret=_thisAddress<address ? _thisAddress : 0;
  uint32_t allowed=millis()>liveSpan ? millis()-liveSpan : 0;
  for (uint8_t i=0;i<_addressCount;i++){
    if (_address[i].id<address && _address[i].seen>=allowed)
      ret=(ret==0 ? _address[i].id : min(ret,_address[i].id));
  }
  return ret;
}

#define MII_MODEM_CLASS MiiRFClass
#endif