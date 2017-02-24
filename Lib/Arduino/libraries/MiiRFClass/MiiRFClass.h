#ifndef MiiRFClass_h
#define MiiRFClass_h
/**
MiiRFClass is build on the RadioHead library with options is needed to do time
synchronization between 2 or devices. It also implements a own Acknowledge
system and buffering of collision messages.
It has been combined into one library because it will save 2.5kb of mem

TODO: SPLIT CODE SO IT CAN BE USED IN MiiRF also using a const to replace name

In future we should see if we can use  uint32_t getLastPreambleTime() instead of _rxTime
*/
#include <Mii.h>
#include <SPI.h>

#define MII_SUPPORT_CHANNEL_SET 0
//Time used to send next TimeValidation
#define MII_TIME_INTERVAL_CLIENT 30000
//Time used to request time if not synced
#define MII_TIME_INTERVAL_MASTER 7500
//Time used to request time if we are using notimesync
#define MII_TIME_INTERVAL_NOTIMESYNC 4000

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
//Request Time command
#define MII_C_REQ_TIME  0xFC
//Request Change Channel
#define MII_C_CHANNEL   0xFB


// The acknowledgement bit in the FLAGS
#define MII_RF_FLAGS_ACK 0x80

//Redefine RH to MII
#define MII_BROADCAST_ADDRESS 0xFF

//Create a unique name for each class and set constants depending on types
#define MII_TXPOW_1DBM                         0x00
#define MII_TXPOW_2DBM                         0x01
#define MII_TXPOW_5DBM                         0x02
#define MII_TXPOW_8DBM                         0x03
#define MII_TXPOW_11DBM                        0x04
#define MII_TXPOW_14DBM                        0x05
#define MII_TXPOW_17DBM                        0x06
#define MII_TXPOW_20DBM                        0x07

//The number of ms between two messages
#define MII_RF_MIN_MSG_INTERVAL 70

//Setting used FSK_Rb2Fd5 transmit time is 70 ms so we take 5 times just to be sure (inc processing)
#define MII_RF_DEF_TIMEOUT MII_RF_MIN_MSG_INTERVAL*5

//Additionel time before sending again after send, allowing other systems to process
#define MII_RF_SEND_DELAY 15

//Check if the message size is big enough to receive timing_t message ()
#if  MII_RF_MESSAGE_LEN  > 55
  #error The MII_RF_MESSAGE_LEN is to small to support timing message for this modem
#endif

//We support maximum 2 collision messages, setting to 0 will disable and save 500bytes
#define MII_MAX_COLLISIONS 2

#ifndef MII_MAX_ADDRESS
//We support maximum 8 address connected to, setting 0 will disable address caching
#define MII_MAX_ADDRESS   8
#endif
//The default live span of a address we have seen in msseconds
#define MII_ADDRESS_LIVESPAN  600000L


//-------- ORIGINAL RF22 -------------------------- START
//Setting modem config to fixed will save 600 bytes
//#define MII_RF_FIXED_CONFIG { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x22, 0x08 } //FSK 2, 5

// This is the maximum number of interrupts the library can support
// Most Arduinos can handle 2, Megas can handle more
#define MII_RF_NUM_INTERRUPTS 3

// This is the bit in the SPI address that marks it as a write
#define MII_RF_SPI_WRITE_MASK 0x80

// Max number of octets the RF22 Rx and Tx FIFOs can hold
#define MII_RF_FIFO_SIZE 64

// Keep track of the mode the RF22 is in
#define MII_RF_MODE_IDLE         0
#define MII_RF_MODE_RX           1
#define MII_RF_MODE_TX           2

// These values we set for FIFO thresholds (4, 55) are actually the same as the POR values
#define MII_RF_TXFFAEM_THRESHOLD 4
#define MII_RF_RXFFAFULL_THRESHOLD 55

// This is the default node address,
#define MII_RF_DEFAULT_NODE_ADDRESS 0

// This address in the TO addreess signifies a broadcast
#define MII_RF_BROADCAST_ADDRESS MII_BROADCAST_ADDRESS

// Number of registers to be passed to setModemConfig(). Obsolete.
#define MII_RF_NUM_MODEM_CONFIG_REGS 18

// Register names
#define MII_RF_REG_00_DEVICE_TYPE                         0x00
#define MII_RF_REG_01_VERSION_CODE                        0x01
#define MII_RF_REG_02_DEVICE_STATUS                       0x02
#define MII_RF_REG_03_INTERRUPT_STATUS1                   0x03
#define MII_RF_REG_04_INTERRUPT_STATUS2                   0x04
#define MII_RF_REG_05_INTERRUPT_ENABLE1                   0x05
#define MII_RF_REG_06_INTERRUPT_ENABLE2                   0x06
#define MII_RF_REG_07_OPERATING_MODE1                     0x07
#define MII_RF_REG_08_OPERATING_MODE2                     0x08
#define MII_RF_REG_09_OSCILLATOR_LOAD_CAPACITANCE         0x09
#define MII_RF_REG_0A_UC_OUTPUT_CLOCK                     0x0a
#define MII_RF_REG_0B_GPIO_CONFIGURATION0                 0x0b
#define MII_RF_REG_0C_GPIO_CONFIGURATION1                 0x0c
#define MII_RF_REG_0D_GPIO_CONFIGURATION2                 0x0d
#define MII_RF_REG_0E_IO_PORT_CONFIGURATION               0x0e
#define MII_RF_REG_0F_ADC_CONFIGURATION                   0x0f
#define MII_RF_REG_10_ADC_SENSOR_AMP_OFFSET               0x10
#define MII_RF_REG_11_ADC_VALUE                           0x11
#define MII_RF_REG_12_TEMPERATURE_SENSOR_CALIBRATION      0x12
#define MII_RF_REG_13_TEMPERATURE_VALUE_OFFSET            0x13
#define MII_RF_REG_14_WAKEUP_TIMER_PERIOD1                0x14
#define MII_RF_REG_15_WAKEUP_TIMER_PERIOD2                0x15
#define MII_RF_REG_16_WAKEUP_TIMER_PERIOD3                0x16
#define MII_RF_REG_17_WAKEUP_TIMER_VALUE1                 0x17
#define MII_RF_REG_18_WAKEUP_TIMER_VALUE2                 0x18
#define MII_RF_REG_19_LDC_MODE_DURATION                   0x19
#define MII_RF_REG_1A_LOW_BATTERY_DETECTOR_THRESHOLD      0x1a
#define MII_RF_REG_1B_BATTERY_VOLTAGE_LEVEL               0x1b
#define MII_RF_REG_1C_IF_FILTER_BANDWIDTH                 0x1c
#define MII_RF_REG_1D_AFC_LOOP_GEARSHIFT_OVERRIDE         0x1d
#define MII_RF_REG_1E_AFC_TIMING_CONTROL                  0x1e
#define MII_RF_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE   0x1f
#define MII_RF_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE    0x20
#define MII_RF_REG_21_CLOCK_RECOVERY_OFFSET2              0x21
#define MII_RF_REG_22_CLOCK_RECOVERY_OFFSET1              0x22
#define MII_RF_REG_23_CLOCK_RECOVERY_OFFSET0              0x23
#define MII_RF_REG_24_CLOCK_RECOVERY_TIMING_LOOP_GAIN1    0x24
#define MII_RF_REG_25_CLOCK_RECOVERY_TIMING_LOOP_GAIN0    0x25
#define MII_RF_REG_26_RSSI                                0x26
#define MII_RF_REG_27_RSSI_THRESHOLD                      0x27
#define MII_RF_REG_28_ANTENNA_DIVERSITY1                  0x28
#define MII_RF_REG_29_ANTENNA_DIVERSITY2                  0x29
#define MII_RF_REG_2A_AFC_LIMITER                         0x2a
#define MII_RF_REG_2B_AFC_CORRECTION_READ                 0x2b
#define MII_RF_REG_2C_OOK_COUNTER_VALUE_1                 0x2c
#define MII_RF_REG_2D_OOK_COUNTER_VALUE_2                 0x2d
#define MII_RF_REG_2E_SLICER_PEAK_HOLD                    0x2e
#define MII_RF_REG_30_DATA_ACCESS_CONTROL                 0x30
#define MII_RF_REG_31_EZMAC_STATUS                        0x31
#define MII_RF_REG_32_HEADER_CONTROL1                     0x32
#define MII_RF_REG_33_HEADER_CONTROL2                     0x33
#define MII_RF_REG_34_PREAMBLE_LENGTH                     0x34
#define MII_RF_REG_35_PREAMBLE_DETECTION_CONTROL1         0x35
#define MII_RF_REG_36_SYNC_WORD3                          0x36
#define MII_RF_REG_37_SYNC_WORD2                          0x37
#define MII_RF_REG_38_SYNC_WORD1                          0x38
#define MII_RF_REG_39_SYNC_WORD0                          0x39
#define MII_RF_REG_3A_TRANSMIT_HEADER3                    0x3a
#define MII_RF_REG_3B_TRANSMIT_HEADER2                    0x3b
#define MII_RF_REG_3C_TRANSMIT_HEADER1                    0x3c
#define MII_RF_REG_3D_TRANSMIT_HEADER0                    0x3d
#define MII_RF_REG_3E_PACKET_LENGTH                       0x3e
#define MII_RF_REG_3F_CHECK_HEADER3                       0x3f
#define MII_RF_REG_40_CHECK_HEADER2                       0x40
#define MII_RF_REG_41_CHECK_HEADER1                       0x41
#define MII_RF_REG_42_CHECK_HEADER0                       0x42
#define MII_RF_REG_43_HEADER_ENABLE3                      0x43
#define MII_RF_REG_44_HEADER_ENABLE2                      0x44
#define MII_RF_REG_45_HEADER_ENABLE1                      0x45
#define MII_RF_REG_46_HEADER_ENABLE0                      0x46
#define MII_RF_REG_47_RECEIVED_HEADER3                    0x47
#define MII_RF_REG_48_RECEIVED_HEADER2                    0x48
#define MII_RF_REG_49_RECEIVED_HEADER1                    0x49
#define MII_RF_REG_4A_RECEIVED_HEADER0                    0x4a
#define MII_RF_REG_4B_RECEIVED_PACKET_LENGTH              0x4b
#define MII_RF_REG_50_ANALOG_TEST_BUS_SELECT              0x50
#define MII_RF_REG_51_DIGITAL_TEST_BUS_SELECT             0x51
#define MII_RF_REG_52_TX_RAMP_CONTROL                     0x52
#define MII_RF_REG_53_PLL_TUNE_TIME                       0x53
#define MII_RF_REG_55_CALIBRATION_CONTROL                 0x55
#define MII_RF_REG_56_MODEM_TEST                          0x56
#define MII_RF_REG_57_CHARGE_PUMP_TEST                    0x57
#define MII_RF_REG_58_CHARGE_PUMP_CURRENT_TRIMMING        0x58
#define MII_RF_REG_59_DIVIDER_CURRENT_TRIMMING            0x59
#define MII_RF_REG_5A_VCO_CURRENT_TRIMMING                0x5a
#define MII_RF_REG_5B_VCO_CALIBRATION                     0x5b
#define MII_RF_REG_5C_SYNTHESIZER_TEST                    0x5c
#define MII_RF_REG_5D_BLOCK_ENABLE_OVERRIDE1              0x5d
#define MII_RF_REG_5E_BLOCK_ENABLE_OVERRIDE2              0x5e
#define MII_RF_REG_5F_BLOCK_ENABLE_OVERRIDE3              0x5f
#define MII_RF_REG_60_CHANNEL_FILTER_COEFFICIENT_ADDRESS  0x60
#define MII_RF_REG_61_CHANNEL_FILTER_COEFFICIENT_VALUE    0x61
#define MII_RF_REG_62_CRYSTAL_OSCILLATOR_POR_CONTROL      0x62
#define MII_RF_REG_63_RC_OSCILLATOR_COARSE_CALIBRATION    0x63
#define MII_RF_REG_64_RC_OSCILLATOR_FINE_CALIBRATION      0x64
#define MII_RF_REG_65_LDO_CONTROL_OVERRIDE                0x65
#define MII_RF_REG_66_LDO_LEVEL_SETTINGS                  0x66
#define MII_RF_REG_67_DELTA_SIGMA_ADC_TUNING1             0x67
#define MII_RF_REG_68_DELTA_SIGMA_ADC_TUNING2             0x68
#define MII_RF_REG_69_AGC_OVERRIDE1                       0x69
#define MII_RF_REG_6A_AGC_OVERRIDE2                       0x6a
#define MII_RF_REG_6B_GFSK_FIR_FILTER_COEFFICIENT_ADDRESS 0x6b
#define MII_RF_REG_6C_GFSK_FIR_FILTER_COEFFICIENT_VALUE   0x6c
#define MII_RF_REG_6D_TX_POWER                            0x6d
#define MII_RF_REG_6E_TX_DATA_RATE1                       0x6e
#define MII_RF_REG_6F_TX_DATA_RATE0                       0x6f
#define MII_RF_REG_70_MODULATION_CONTROL1                 0x70
#define MII_RF_REG_71_MODULATION_CONTROL2                 0x71
#define MII_RF_REG_72_FREQUENCY_DEVIATION                 0x72
#define MII_RF_REG_73_FREQUENCY_OFFSET1                   0x73
#define MII_RF_REG_74_FREQUENCY_OFFSET2                   0x74
#define MII_RF_REG_75_FREQUENCY_BAND_SELECT               0x75
#define MII_RF_REG_76_NOMINAL_CARRIER_FREQUENCY1          0x76
#define MII_RF_REG_77_NOMINAL_CARRIER_FREQUENCY0          0x77
#define MII_RF_REG_79_FREQUENCY_HOPPING_CHANNEL_SELECT    0x79
#define MII_RF_REG_7A_FREQUENCY_HOPPING_STEP_SIZE         0x7a
#define MII_RF_REG_7C_TX_FIFO_CONTROL1                    0x7c
#define MII_RF_REG_7D_TX_FIFO_CONTROL2                    0x7d
#define MII_RF_REG_7E_RX_FIFO_CONTROL                     0x7e
#define MII_RF_REG_7F_FIFO_ACCESS                         0x7f

// These register masks etc are named wherever possible
// corresponding to the bit and field names in the RF-22 Manual
// MII_RF_REG_00_DEVICE_TYPE                      0x00
#define MII_RF_DEVICE_TYPE_RX_TRX                 0x08
#define MII_RF_DEVICE_TYPE_TX                     0x07

// MII_RF_REG_02_DEVICE_STATUS                    0x02
#define MII_RF_FFOVL                              0x80
#define MII_RF_FFUNFL                             0x40
#define MII_RF_RXFFEM                             0x20
#define MII_RF_HEADERR                            0x10
#define MII_RF_FREQERR                            0x08
#define MII_RF_LOCKDET                            0x04
#define MII_RF_CPS                                0x03
#define MII_RF_CPS_IDLE                           0x00
#define MII_RF_CPS_RX                             0x01
#define MII_RF_CPS_TX                             0x10

// MII_RF_REG_03_INTERRUPT_STATUS1                0x03
#define MII_RF_IFFERROR                           0x80
#define MII_RF_ITXFFAFULL                         0x40
#define MII_RF_ITXFFAEM                           0x20
#define MII_RF_IRXFFAFULL                         0x10
#define MII_RF_IEXT                               0x08
#define MII_RF_IPKSENT                            0x04
#define MII_RF_IPKVALID                           0x02
#define MII_RF_ICRCERROR                          0x01

// MII_RF_REG_04_INTERRUPT_STATUS2                0x04
#define MII_RF_ISWDET                             0x80
#define MII_RF_IPREAVAL                           0x40
#define MII_RF_IPREAINVAL                         0x20
#define MII_RF_IRSSI                              0x10
#define MII_RF_IWUT                               0x08
#define MII_RF_ILBD                               0x04
#define MII_RF_ICHIPRDY                           0x02
#define MII_RF_IPOR                               0x01

// MII_RF_REG_05_INTERRUPT_ENABLE1                0x05
#define MII_RF_ENFFERR                            0x80
#define MII_RF_ENTXFFAFULL                        0x40
#define MII_RF_ENTXFFAEM                          0x20
#define MII_RF_ENRXFFAFULL                        0x10
#define MII_RF_ENEXT                              0x08
#define MII_RF_ENPKSENT                           0x04
#define MII_RF_ENPKVALID                          0x02
#define MII_RF_ENCRCERROR                         0x01

// MII_RF_REG_06_INTERRUPT_ENABLE2                0x06
#define MII_RF_ENSWDET                            0x80
#define MII_RF_ENPREAVAL                          0x40
#define MII_RF_ENPREAINVAL                        0x20
#define MII_RF_ENRSSI                             0x10
#define MII_RF_ENWUT                              0x08
#define MII_RF_ENLBDI                             0x04
#define MII_RF_ENCHIPRDY                          0x02
#define MII_RF_ENPOR                              0x01

// MII_RF_REG_07_OPERATING_MODE                   0x07
#define MII_RF_SWRES                              0x80
#define MII_RF_ENLBD                              0x40
#define MII_RF_ENWT                               0x20
#define MII_RF_X32KSEL                            0x10
#define MII_RF_TXON                               0x08
#define MII_RF_RXON                               0x04
#define MII_RF_PLLON                              0x02
#define MII_RF_XTON                               0x01

// MII_RF_REG_08_OPERATING_MODE2                  0x08
#define MII_RF_ANTDIV                             0xc0
#define MII_RF_RXMPK                              0x10
#define MII_RF_AUTOTX                             0x08
#define MII_RF_ENLDM                              0x04
#define MII_RF_FFCLRRX                            0x02
#define MII_RF_FFCLRTX                            0x01

// MII_RF_REG_0F_ADC_CONFIGURATION                0x0f
#define MII_RF_ADCSTART                           0x80
#define MII_RF_ADCDONE                            0x80
#define MII_RF_ADCSEL                             0x70
#define MII_RF_ADCSEL_INTERNAL_TEMPERATURE_SENSOR 0x00
#define MII_RF_ADCSEL_GPIO0_SINGLE_ENDED          0x10
#define MII_RF_ADCSEL_GPIO1_SINGLE_ENDED          0x20
#define MII_RF_ADCSEL_GPIO2_SINGLE_ENDED          0x30
#define MII_RF_ADCSEL_GPIO0_GPIO1_DIFFERENTIAL    0x40
#define MII_RF_ADCSEL_GPIO1_GPIO2_DIFFERENTIAL    0x50
#define MII_RF_ADCSEL_GPIO0_GPIO2_DIFFERENTIAL    0x60
#define MII_RF_ADCSEL_GND                         0x70
#define MII_RF_ADCREF                             0x0c
#define MII_RF_ADCREF_BANDGAP_VOLTAGE             0x00
#define MII_RF_ADCREF_VDD_ON_3                    0x08
#define MII_RF_ADCREF_VDD_ON_2                    0x0c
#define MII_RF_ADCGAIN                            0x03

// MII_RF_REG_10_ADC_SENSOR_AMP_OFFSET            0x10
#define MII_RF_ADCOFFS                            0x0f

// MII_RF_REG_12_TEMPERATURE_SENSOR_CALIBRATION   0x12
#define MII_RF_TSRANGE                            0xc0
#define MII_RF_TSRANGE_M64_64C                    0x00
#define MII_RF_TSRANGE_M64_192C                   0x40
#define MII_RF_TSRANGE_0_128C                     0x80
#define MII_RF_TSRANGE_M40_216F                   0xc0
#define MII_RF_ENTSOFFS                           0x20
#define MII_RF_ENTSTRIM                           0x10
#define MII_RF_TSTRIM                             0x0f

// MII_RF_REG_14_WAKEUP_TIMER_PERIOD1             0x14
#define MII_RF_WTR                                0x3c
#define MII_RF_WTD                                0x03

// MII_RF_REG_1D_AFC_LOOP_GEARSHIFT_OVERRIDE      0x1d
#define MII_RF_AFBCD                              0x80
#define MII_RF_ENAFC                              0x40
#define MII_RF_AFCGEARH                           0x38
#define MII_RF_AFCGEARL                           0x07

// MII_RF_REG_1E_AFC_TIMING_CONTROL               0x1e
#define MII_RF_SWAIT_TIMER                        0xc0
#define MII_RF_SHWAIT                             0x38
#define MII_RF_ANWAIT                             0x07

// MII_RF_REG_30_DATA_ACCESS_CONTROL              0x30
#define MII_RF_ENPACRX                            0x80
#define MII_RF_MSBFRST                            0x00
#define MII_RF_LSBFRST                            0x40
#define MII_RF_CRCHDRS                            0x00
#define MII_RF_CRCDONLY                           0x20
#define MII_RF_SKIP2PH                            0x10
#define MII_RF_ENPACTX                            0x08
#define MII_RF_ENCRC                              0x04
#define MII_RF_CRC                                0x03
#define MII_RF_CRC_CCITT                          0x00
#define MII_RF_CRC_CRC_16_IBM                     0x01
#define MII_RF_CRC_IEC_16                         0x02
#define MII_RF_CRC_BIACHEVA                       0x03

// MII_RF_REG_32_HEADER_CONTROL1                  0x32
#define MII_RF_BCEN                               0xf0
#define MII_RF_BCEN_NONE                          0x00
#define MII_RF_BCEN_HEADER0                       0x10
#define MII_RF_BCEN_HEADER1                       0x20
#define MII_RF_BCEN_HEADER2                       0x40
#define MII_RF_BCEN_HEADER3                       0x80
#define MII_RF_HDCH                               0x0f
#define MII_RF_HDCH_NONE                          0x00
#define MII_RF_HDCH_HEADER0                       0x01
#define MII_RF_HDCH_HEADER1                       0x02
#define MII_RF_HDCH_HEADER2                       0x04
#define MII_RF_HDCH_HEADER3                       0x08

// MII_RF_REG_33_HEADER_CONTROL2                  0x33
#define MII_RF_HDLEN                              0x70
#define MII_RF_HDLEN_0                            0x00
#define MII_RF_HDLEN_1                            0x10
#define MII_RF_HDLEN_2                            0x20
#define MII_RF_HDLEN_3                            0x30
#define MII_RF_HDLEN_4                            0x40
#define MII_RF_VARPKLEN                           0x00
#define MII_RF_FIXPKLEN                           0x08
#define MII_RF_SYNCLEN                            0x06
#define MII_RF_SYNCLEN_1                          0x00
#define MII_RF_SYNCLEN_2                          0x02
#define MII_RF_SYNCLEN_3                          0x04
#define MII_RF_SYNCLEN_4                          0x06
#define MII_RF_PREALEN8                           0x01

// MII_RF_REG_6D_TX_POWER                         0x6d
#define MII_RF_TXPOW                              0x07
#define MII_RF_TXPOW_4X31                         0x08 // Not used in RFM22B
#define MII_RF_TXPOW_1DBM                         0x00
#define MII_RF_TXPOW_2DBM                         0x01
#define MII_RF_TXPOW_5DBM                         0x02
#define MII_RF_TXPOW_8DBM                         0x03
#define MII_RF_TXPOW_11DBM                        0x04
#define MII_RF_TXPOW_14DBM                        0x05 // +28dBm on RF23bp
#define MII_RF_TXPOW_17DBM                        0x06 // +29dBm on RF23bp
#define MII_RF_TXPOW_20DBM                        0x07 // +30dBm on RF23bp
// IN RFM23B
#define MII_RF_TXPOW_LNA_SW                       0x08

// MII_RF_REG_71_MODULATION_CONTROL2              0x71
#define MII_RF_TRCLK                              0xc0
#define MII_RF_TRCLK_NONE                         0x00
#define MII_RF_TRCLK_GPIO                         0x40
#define MII_RF_TRCLK_SDO                          0x80
#define MII_RF_TRCLK_NIRQ                         0xc0
#define MII_RF_DTMOD                              0x30
#define MII_RF_DTMOD_DIRECT_GPIO                  0x00
#define MII_RF_DTMOD_DIRECT_SDI                   0x10
#define MII_RF_DTMOD_FIFO                         0x20
#define MII_RF_DTMOD_PN9                          0x30
#define MII_RF_ENINV                              0x08
#define MII_RF_FD8                                0x04
#define MII_RF_MODTYP                             0x30
#define MII_RF_MODTYP_UNMODULATED                 0x00
#define MII_RF_MODTYP_OOK                         0x01
#define MII_RF_MODTYP_FSK                         0x02
#define MII_RF_MODTYP_GFSK                        0x03

// MII_RF_REG_75_FREQUENCY_BAND_SELECT            0x75
#define MII_RF_SBSEL                              0x40
#define MII_RF_HBSEL                              0x20
#define MII_RF_FB                                 0x1f

//-------- ORIGINAL RF22 -------------------------- END

class MiiRFClass {
public:
  ///Init the based independent of RF device used
  MiiRFClass(uint8_t slaveSelectPin = MII_PIN_RF_CS, uint8_t interruptPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN);

   /// \brief Defines register values for a set of modem configuration registers
    ///
    /// Defines register values for a set of modem configuration registers
    /// that can be passed to setModemConfig()
    /// if none of the choices in ModemConfigChoice suit your need
    /// setModemConfig() writes the register values to the appropriate RF22 registers
    /// to set the desired modulation type, data rate and deviation/bandwidth.
    /// Suitable values for these registers can be computed using the register calculator at
    /// http://www.hoperf.com/upload/rf/RF22B%2023B%2031B%2042B%2043B%20Register%20Settings_RevB1-v5.xls
    typedef struct
    {
	uint8_t    reg_1c;   ///< Value for register MII_RF_REG_1C_IF_FILTER_BANDWIDTH
	uint8_t    reg_1f;   ///< Value for register MII_RF_REG_1F_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE
	uint8_t    reg_20;   ///< Value for register MII_RF_REG_20_CLOCK_RECOVERY_OVERSAMPLING_RATE
	uint8_t    reg_21;   ///< Value for register MII_RF_REG_21_CLOCK_RECOVERY_OFFSET2
	uint8_t    reg_22;   ///< Value for register MII_RF_REG_22_CLOCK_RECOVERY_OFFSET1
	uint8_t    reg_23;   ///< Value for register MII_RF_REG_23_CLOCK_RECOVERY_OFFSET0
	uint8_t    reg_24;   ///< Value for register MII_RF_REG_24_CLOCK_RECOVERY_TIMING_LOOP_GAIN1
	uint8_t    reg_25;   ///< Value for register MII_RF_REG_25_CLOCK_RECOVERY_TIMING_LOOP_GAIN0
	uint8_t    reg_2c;   ///< Value for register MII_RF_REG_2C_OOK_COUNTER_VALUE_1
	uint8_t    reg_2d;   ///< Value for register MII_RF_REG_2D_OOK_COUNTER_VALUE_2
	uint8_t    reg_2e;   ///< Value for register MII_RF_REG_2E_SLICER_PEAK_HOLD
	uint8_t    reg_58;   ///< Value for register MII_RF_REG_58_CHARGE_PUMP_CURRENT_TRIMMING
	uint8_t    reg_69;   ///< Value for register MII_RF_REG_69_AGC_OVERRIDE1
	uint8_t    reg_6e;   ///< Value for register MII_RF_REG_6E_TX_DATA_RATE1
	uint8_t    reg_6f;   ///< Value for register MII_RF_REG_6F_TX_DATA_RATE0
	uint8_t    reg_70;   ///< Value for register MII_RF_REG_70_MODULATION_CONTROL1
	uint8_t    reg_71;   ///< Value for register MII_RF_REG_71_MODULATION_CONTROL2
	uint8_t    reg_72;   ///< Value for register MII_RF_REG_72_FREQUENCY_DEVIATION
    } ModemConfig;

    /// Choices for setModemConfig() for a selected subset of common modulation types,
    /// and data rates. If you need another configuration, use the register calculator.
    /// and call setModemRegisters() with your desired settings
    /// These are indexes into MODEM_CONFIG_TABLE
    typedef enum
    {
	UnmodulatedCarrier = 0, ///< Unmodulated carrier for testing
	FSK_PN9_Rb2Fd5,      ///< FSK, No Manchester, Rb = 2kbs, Fd = 5kHz, PN9 random modulation for testing

	FSK_Rb2Fd5,	     ///< FSK, No Manchester, Rb = 2kbs,    Fd = 5kHz
	FSK_Rb2_4Fd36,       ///< FSK, No Manchester, Rb = 2.4kbs,  Fd = 36kHz
	FSK_Rb4_8Fd45,       ///< FSK, No Manchester, Rb = 4.8kbs,  Fd = 45kHz
	FSK_Rb9_6Fd45,       ///< FSK, No Manchester, Rb = 9.6kbs,  Fd = 45kHz
	FSK_Rb19_2Fd9_6,     ///< FSK, No Manchester, Rb = 19.2kbs, Fd = 9.6kHz
	FSK_Rb38_4Fd19_6,    ///< FSK, No Manchester, Rb = 38.4kbs, Fd = 19.6kHz
	FSK_Rb57_6Fd28_8,    ///< FSK, No Manchester, Rb = 57.6kbs, Fd = 28.8kHz
	FSK_Rb125Fd125,      ///< FSK, No Manchester, Rb = 125kbs,  Fd = 125kHz
	FSK_Rb_512Fd2_5,     ///< FSK, No Manchester, Rb = 512bs,  Fd = 2.5kHz, for POCSAG compatibility
	FSK_Rb_512Fd4_5,     ///< FSK, No Manchester, Rb = 512bs,  Fd = 4.5kHz, for POCSAG compatibility

	GFSK_Rb2Fd5,         ///< GFSK, No Manchester, Rb = 2kbs,    Fd = 5kHz
	GFSK_Rb2_4Fd36,      ///< GFSK, No Manchester, Rb = 2.4kbs,  Fd = 36kHz
	GFSK_Rb4_8Fd45,      ///< GFSK, No Manchester, Rb = 4.8kbs,  Fd = 45kHz
	GFSK_Rb9_6Fd45,      ///< GFSK, No Manchester, Rb = 9.6kbs,  Fd = 45kHz
	GFSK_Rb19_2Fd9_6,    ///< GFSK, No Manchester, Rb = 19.2kbs, Fd = 9.6kHz
	GFSK_Rb38_4Fd19_6,   ///< GFSK, No Manchester, Rb = 38.4kbs, Fd = 19.6kHz
	GFSK_Rb57_6Fd28_8,   ///< GFSK, No Manchester, Rb = 57.6kbs, Fd = 28.8kHz
	GFSK_Rb125Fd125,     ///< GFSK, No Manchester, Rb = 125kbs,  Fd = 125kHz

	OOK_Rb1_2Bw75,       ///< OOK, No Manchester, Rb = 1.2kbs,  Rx Bandwidth = 75kHz
	OOK_Rb2_4Bw335,      ///< OOK, No Manchester, Rb = 2.4kbs,  Rx Bandwidth = 335kHz
	OOK_Rb4_8Bw335,      ///< OOK, No Manchester, Rb = 4.8kbs,  Rx Bandwidth = 335kHz
	OOK_Rb9_6Bw335,      ///< OOK, No Manchester, Rb = 9.6kbs,  Rx Bandwidth = 335kHz
	OOK_Rb19_2Bw335,     ///< OOK, No Manchester, Rb = 19.2kbs, Rx Bandwidth = 335kHz
	OOK_Rb38_4Bw335,     ///< OOK, No Manchester, Rb = 38.4kbs, Rx Bandwidth = 335kHz
	OOK_Rb40Bw335        ///< OOK, No Manchester, Rb = 40kbs,   Rx Bandwidth = 335kHz
    } ModemConfigChoice;

    /// \brief Defines the available choices for CRC
    /// Types of permitted CRC polynomials, to be passed to setCRCPolynomial()
    /// They deliberately have the same numeric values as the crc[1:0] field of Register
    /// MII_RF_REG_30_DATA_ACCESS_CONTROL
    typedef enum
    {
	CRC_CCITT = 0,       ///< CCITT
	CRC_16_IBM = 1,      ///< CRC-16 (IBM) The default used by RF22 library
	CRC_IEC_16 = 2,      ///< IEC-16
	CRC_Biacheva = 3     ///< Biacheva
    } CRCPolynomial;

    /// Initialises this instance and the radio module connected to it.
    /// The following steps are taken:
    /// - Initialise the slave select pin and the SPI interface library
    /// - Software reset the RF22 module
    /// - Checks the connected RF22 module is either a MII_RF_DEVICE_TYPE_RX_TRX or a MII_RF_DEVICE_TYPE_TX
    /// - Attaches an interrupt handler
    /// - Configures the RF22 module
    /// - Sets the frequency to 434.0 MHz
    /// - Sets the modem data rate to FSK_Rb2_4Fd36
    /// \return  true if everything was successful
    bool        init();

    /// Issues a software reset to the
    /// RF22 module. Blocks for 1ms to ensure the reset is complete.
    void           reset();

    /// Reads a single register from the RF22
    /// \param[in] reg Register number, one of MII_RF_REG_*
    /// \return The value of the register
    uint8_t        spiRead(uint8_t reg);

    /// Writes a single byte to the RF22
    /// \param[in] reg Register number, one of MII_RF_REG_*
    /// \param[in] val The value to write
    void           spiWrite(uint8_t reg, uint8_t val);

    /// Reads a number of consecutive registers from the RF22 using burst read mode
    /// \param[in] reg Register number of the first register, one of MII_RF_REG_*
    /// \param[in] dest Array to write the register values to. Must be at least len bytes
    /// \param[in] len Number of bytes to read
    void           spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len);

    /// Write a number of consecutive registers using burst write mode
    /// \param[in] reg Register number of the first register, one of MII_RF_REG_*
    /// \param[in] src Array of new register values to write. Must be at least len bytes
    /// \param[in] len Number of bytes to write
    void           spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len);

    /// Reads and returns the device status register MII_RF_REG_02_DEVICE_STATUS
    /// \return The value of the device status register
    uint8_t        statusRead();

    /// Reads a value from the on-chip analog-digital converter
    /// \param[in] adcsel Selects the ADC input to measure. One of MII_RF_ADCSEL_*. Defaults to the
    /// internal temperature sensor
    /// \param[in] adcref Specifies the refernce voltage to use. One of MII_RF_ADCREF_*.
    /// Defaults to the internal bandgap voltage.
    /// \param[in] adcgain Amplifier gain selection.
    /// \param[in] adcoffs Amplifier offseet (0 to 15).
    /// \return The analog value. 0 to 255.
    uint8_t        adcRead(uint8_t adcsel = MII_RF_ADCSEL_INTERNAL_TEMPERATURE_SENSOR,
			   uint8_t adcref = MII_RF_ADCREF_BANDGAP_VOLTAGE,
			   uint8_t adcgain = 0,
			   uint8_t adcoffs = 0);

    /// Reads the on-chip temperature sensoer
    /// \param[in] tsrange Specifies the temperature range to use. One of MII_RF_TSRANGE_*
    /// \param[in] tvoffs Specifies the temperature value offset. This is actually signed value
    /// added to the measured temperature value
    /// \return The measured temperature.
    uint8_t        temperatureRead(uint8_t tsrange = MII_RF_TSRANGE_M64_64C, uint8_t tvoffs = 0);

    /// Reads the wakeup timer value in registers MII_RF_REG_17_WAKEUP_TIMER_VALUE1
    /// and MII_RF_REG_18_WAKEUP_TIMER_VALUE2
    /// \return The wakeup timer value
    uint16_t       wutRead();

    /// Sets the wakeup timer period registers MII_RF_REG_14_WAKEUP_TIMER_PERIOD1,
    /// MII_RF_REG_15_WAKEUP_TIMER_PERIOD2 and MII_RF_R<EG_16_WAKEUP_TIMER_PERIOD3
    /// \param[in] wtm Wakeup timer mantissa value
    /// \param[in] wtr Wakeup timer exponent R value
    /// \param[in] wtd Wakeup timer exponent D value
    void           setWutPeriod(uint16_t wtm, uint8_t wtr = 0, uint8_t wtd = 0);

    /// Sets the transmitter and receiver centre frequency
    /// \param[in] centre Frequency in MHz. 240.0 to 960.0. Caution, some versions of RF22 and derivatives
    /// implemented more restricted frequency ranges.
    /// \param[in] afcPullInRange Sets the AF Pull In Range in MHz. Defaults to 0.05MHz (50kHz).
    /// Range is 0.0 to 0.159375
    /// for frequencies 240.0 to 480MHz, and 0.0 to 0.318750MHz for  frequencies 480.0 to 960MHz,
    /// \return true if the selected frquency centre + (fhch * fhs) is within range and the afcPullInRange
    /// is within range
    bool        setFrequency(float centre, float afcPullInRange = 0.05);

    /// Sets the frequency hopping step size.
    /// \param[in] fhs Frequency Hopping step size in 10kHz increments
    /// \return true if centre + (fhch * fhs) is within limits
    bool        setFHStepSize(uint8_t fhs);

    /// Sets the frequncy hopping channel. Adds fhch * fhs to centre frequency
    /// \param[in] fhch The channel number
    /// \return true if the selected frquency centre + (fhch * fhs) is within range
    bool        setFHChannel(uint8_t fhch);

    /// Reads and returns the current RSSI value from register MII_RF_REG_26_RSSI. If you want to find the RSSI
    /// of the last received message, use lastRssi() instead.
    /// \return The current RSSI value
    uint8_t        rssiRead();

    /// Reads and returns the current EZMAC value from register MII_RF_REG_31_EZMAC_STATUS
    /// \return The current EZMAC value
    uint8_t        ezmacStatusRead();

    /// Sets the parameters for the RF22 Idle mode in register MII_RF_REG_07_OPERATING_MODE.
    /// Idle mode is the mode the RF22 will be in when not transmitting or receiving. The default idle mode
    /// is MII_RF_XTON ie READY mode.
    /// \param[in] mode Mask of mode bits, using MII_RF_SWRES, MII_RF_ENLBD, MII_RF_ENWT,
    /// MII_RF_X32KSEL, MII_RF_PLLON, MII_RF_XTON.
    void           setMode(uint8_t mode);

    /// If current mode is Rx or Tx changes it to Idle. If the transmitter or receiver is running,
    /// disables them.
    void           setModeIdle();

    /// If current mode is Tx or Idle, changes it to Rx.
    /// Starts the receiver in the RF22.
    void           setModeRx();

    /// If current mode is Rx or Idle, changes it to Rx.
    /// Starts the transmitter in the RF22.
    void           setModeTx();

    /// Returns the operating mode of the library.
    /// \return the current mode, one of MII_RF_MODE_*
    uint8_t        mode();

    /// Sets the transmitter power output level in register MII_RF_REG_6D_TX_POWER.
    /// Be a good neighbour and set the lowest power level you need.
    /// After init(), the power will be set to MII_RF_TXPOW_8DBM.
    /// Caution: In some countries you may only select MII_RF_TXPOW_17DBM if you
    /// are also using frequency hopping.
    /// \param[in] power Transmitter power level, one of MII_RF_TXPOW_*
    void           setTxPower(uint8_t power);

    /// Sets all the registered required to configure the data modem in the RF22, including the data rate,
    /// bandwidths etc. You cas use this to configure the modem with custom configuraitons if none of the
    /// canned configurations in ModemConfigChoice suit you.
    /// \param[in] config A ModemConfig structure containing values for the modem configuration registers.
    void           setModemRegisters(const ModemConfig* config);

    /// Select one of the predefined modem configurations. If you need a modem configuration not provided
    /// here, use setModemRegisters() with your own ModemConfig.
    /// \param[in] index The configuration choice.
    /// \return true if index is a valid choice.
    bool        setModemConfig(ModemConfigChoice index);

    /// Tests whether a new message is available
    /// \return true if a new, complete, error-free uncollected message is available to be retreived by recv().
    bool available(bool allowCollision=true);


    /// Starts the receiver and blocks until a valid received
    /// message is available.
    void           waitAvailable();

    /// Starts the receiver and blocks until a received message is available or a timeout
    /// \param[in] timeout Maximum time to wait in milliseconds.
    /// \return true if a message is available
    bool           waitAvailableTimeout(uint16_t timeout);

    /// Blocks until the RF22 is not in mode MII_RF_MODE_TX (ie until the RF22 is not transmitting).
    /// This effectively waits until any previous transmit packet is finished being transmitted.
    void           waitPacketSent();

    /// Blocks until the RF22 is not in mode MII_RF_MODE_TX (ie until the RF22 is not transmitting)
    /// or until the timeout occuers, whichever happens first
    /// \param[in] timeout Maximum time to wait in milliseconds.
    /// \return true if the RF22 is not transmitting any more
    bool           waitPacketSent(uint16_t timeout);

    /// Tells the receiver to accept messages with any TO address, not just messages
    /// addressed to this node or the broadcast address
    /// \param[in] promiscuous true if you wish to receive messages with any TO address
    void           setPromiscuous(bool promiscuous);

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

    /// Returns the RSSI (Receiver Signal Strength Indicator)
    /// of the last received message. This measurement is taken when
    /// the preamble has been received. It is a (non-linear) measure of the received signal strength.
    /// \return The RSSI
    uint8_t        lastRssi();

    /// Sets the length of the preamble
    /// in 4-bit nibbles.
    /// Caution: this should be set to the same
    /// value on all nodes in your network. Default is 8.
    /// Sets the message preamble length in MII_RF_REG_34_PREAMBLE_LENGTH
    /// \param[in] nibbles Preamble length in nibbles of 4 bits each.
    void           setPreambleLength(uint8_t nibbles);

    /// Sets the sync words for transmit and receive in registers MII_RF_REG_36_SYNC_WORD3
    /// to MII_RF_REG_39_SYNC_WORD0
    /// Caution: SyncWords should be set to the same
    /// value on all nodes in your network. Nodes with different SyncWords set will never receive
    /// each others messages, so different SyncWords can be used to isolate different
    /// networks from each other. Default is { 0x2d, 0xd4 }.
    /// \param[in] syncWords Array of sync words, 1 to 4 octets long
    /// \param[in] len Number of sync words to set, 1 to 4.
    void           setSyncWords(const uint8_t* syncWords, uint8_t len);

    /// Sets the CRC polynomial top be used to generare the CRC for both receive and transmit
    /// Must be called before init(), otherwise the default of CRC_16_IBM will be used.
    /// \param[in] polynomial One of RF22::CRCPolynomial choices CRC_*
    /// \return true if polynomial is a valid option for this radio.
    bool setCRCPolynomial(CRCPolynomial polynomial);

    //Return the last valid read time
    uint32_t rxTime(){return _rxTime;}

    /// Turns the receiver on if it not already on.
    /// If there is a valid message available, copy it to buf and return true
    /// else return false.
    /// If a message is copied, *len is set to the length (Caution, 0 length messages are permitted).
    /// You should be sure to call this function frequently enough to not miss any messages
    /// It is recommended that you call it in your main loop.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
  bool recv(uint8_t* buf=0, uint8_t* len=0,bool peek=false);
  bool recv(bool allowCollision,uint8_t* buf=0, uint8_t* len=0,bool peek=false);
  bool recv8_t(uint8_t  &buf,bool allowCollision=true);
  bool recv16_t(uint16_t  &buf,bool allowCollision=true);
  bool recv32_t(uint32_t  &buf,bool allowCollision=true);
  //Discard ad message by receving the message without ack
  bool discard(bool allowCollision=true);
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
  void sendTime(); //Send the time
  virtual bool syncTime(uint8_t address=MII_BROADCAST_ADDRESS);   //Sync the time with a address making  the address the master

  void  power_on_reset();  /// Cycles the Shutdown pin to force the cradio chip to reset
     /// A more easy init procedure
  bool init(uint8_t address,uint16_t syncWords=0x2dd4,uint8_t channel=1,uint8_t power=MII_TXPOW_17DBM,bool isMaster=false,ModemConfigChoice index=FSK_Rb2Fd5);
  void setChannel(uint8_t channel,uint8_t power,ModemConfigChoice index=FSK_Rb2Fd5);
  void setChannel(uint8_t channel);
  void changeChannel(uint8_t channel); //Change the channel only master by sending command to all clients


   #if MII_MAX_ADDRESS
   uint8_t nextAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the next address
   uint8_t prevAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   uint8_t firstAddress(uint8_t address=0,uint32_t liveSpan=MII_ADDRESS_LIVESPAN); //Give the previous address
   #endif

  void setGpioReversed(bool gpioReversed);
  bool isReady(){return _isReady;}
  virtual bool getTimeSync(){return _timeSync;}
  void setTimeSync(bool state){_timeSync=state;}
  //Called when the internal time clock has been changed larger then drift
  virtual void timeChanged(){}

protected:
  static uint8_t      _interruptCount;
  bool _isReady; //Internal flag set high when system is ready
  bool _timeSync; //When set to true no active time sync will be done only on time commands of master

  //Check if modem has data avaiable
  bool hasAvailable();
  //internalProcess allows you to do internal processing of commands during available checks
  virtual bool internalProcess();
  #if MII_MAX_MSG_CACHE
  bool pushMsg(); //Push a received record on to message store, true if record pushed
  bool popMsg(uint8_t* buf=0, uint8_t* len=0,bool peek=false); //Pop a record from the message store, true if record poped
  uint8_t   _msgCount; //The number of messages in buffer
  msgRec_t _msg[MII_MAX_MSG_CACHE]; //The message buffer
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

  uint8_t   _sdnPin;  /// The configured pin connected to the SDN pin of the radio

  #if MII_MAX_ADDRESS
  uint8_t    _addressCount; //The number of addresses in the routing table
  addressRec_t   _address[MII_MAX_ADDRESS];  //The routing addresses
  #endif

  virtual void chipSelectHigh(void);
  virtual void chipSelectLow(void);
  //Called just before a message is pocessed
  virtual bool beforeProcess(void){return true;}
  //Called just before a message is send
  virtual bool beforeSend(const uint8_t* data=0, uint8_t len=0){return true;}
  virtual void afterSend(const uint8_t* data=0, uint8_t len=0){}

    /// This is a low level function to handle the interrupts for one instance of RF22.
    /// Called automatically by isr0() and isr1()
    /// Should not need to be called.
    void           handleInterrupt();

    /// Clears the receiver buffer.
    /// Internal use only
    void           clearRxBuf();

    /// Clears the transmitter buffer
    /// Internal use only
    void           clearTxBuf();

    /// Fills the transmitter buffer with the data of a mesage to be sent
    /// \param[in] data Array of data bytes to be sent (1 to 255)
    /// \param[in] len Number of data bytes in data (> 0)
    /// \return true if the message length is valid
    bool           fillTxBuf(const uint8_t* data, uint8_t len);

    /// Appends the transmitter buffer with the data of a mesage to be sent
    /// \param[in] data Array of data bytes to be sent (0 to 255)
    /// \param[in] len Number of data bytes in data
    /// \return false if the resulting message would exceed MII_RF_MESSAGE_LEN, else true
    bool           appendTxBuf(const uint8_t* data, uint8_t len);

    /// Internal function to load the next fragment of
    /// the current message into the transmitter FIFO
    /// Internal use only
    void           sendNextFragment();

    ///  function to copy the next fragment from
    /// the receiver FIF) into the receiver buffer
    void           readNextFragment();

    /// Clears the RF22 Rx and Tx FIFOs
    /// Internal use only
    void           resetFifos();

    /// Clears the RF22 Rx FIFO
    /// Internal use only
    void           resetRxFifo();

    /// Clears the RF22 Tx FIFO
    /// Internal use only
    void           resetTxFifo();

    /// This function will be called by handleInterrupt() if an RF22 external interrupt occurs.
    /// This can only happen if external interrupts are enabled in the RF22
    /// (which they are not by default).
    /// Subclasses may override this function to get control when  an RF22 external interrupt occurs.
    virtual void   handleExternalInterrupt();

    /// This function will be called by handleInterrupt() if an RF22 wakeup timer interrupt occurs.
    /// This can only happen if wakeup timer interrupts are enabled in the RF22
    /// (which they are not by default).
    /// Subclasses may override this function to get control when  an RF22 wakeup timer interrupt occurs.
    virtual void   handleWakeupTimerInterrupt();

    /// Sets the TO header to be sent in all subsequent messages
    /// \param[in] to The new TO header value
    void           setHeaderTo(uint8_t to);

    /// Sets the FROM header to be sent in all subsequent messages
    /// \param[in] from The new FROM header value
    void           setHeaderFrom(uint8_t from);

    /// Sets the ID header to be sent in all subsequent messages
    /// \param[in] id The new ID header value
    void           setHeaderId(uint8_t id);
    /// Sets the FLAGS header to be sent in all subsequent messages
    /// \param[in] flags The new FLAGS header value
    void           setHeaderFlags(uint8_t flags);

    /// Start the transmission of the contents
    /// of the Tx buffer
    void           startTransmit();

    /// ReStart the transmission of the contents
    /// of the Tx buffer after a atransmission failure
    void           restartTransmit();

    /// Low level interrupt service routine for RF22 connected to interrupt 0
    static void         isr0();

    /// Low level interrupt service routine for RF22 connected to interrupt 1
    static void         isr1();

    /// Low level interrupt service routine for RF22 connected to interrupt 1
    static void         isr2();

    /// Array of instances connected to interrupts 0 and 1
    static MiiRFClass*   _deviceForInterrupt[];

    volatile uint8_t    _mode; // One of MII_RF_MODE_*

    uint8_t             _idleMode; // The radio mode to use when mode is MII_RF_MODE_IDLE
    uint8_t             _slaveSelectPin;
    uint8_t             _interruptPin;
    uint8_t             _deviceType;
    // Message confiuration options
    CRCPolynomial       _polynomial;

    // These volatile members may get changed in the interrupt service routine
    volatile uint8_t    _bufLen;
    uint8_t             _buf[MII_RF_MESSAGE_LEN];

    //The flags
    uint8_t _rxHeaderTo;
	  uint8_t _rxHeaderFrom;
	  uint8_t _rxHeaderId;
	  uint8_t _rxHeaderFlags;
	  uint8_t _txHeaderTo;
	  uint8_t _txHeaderFrom;
	  uint8_t _txHeaderId;
	  uint8_t _txHeaderFlags;
	  uint8_t _thisAddress;
	  bool _isPromiscuous;
    volatile bool    _rxBufValid;
    volatile uint8_t    _txBufSentIndex;
    volatile uint32_t   _rxTime;
    volatile uint8_t    _lastRssi;

    #if SPI_ISOLATE
    uint8_t             _SPCR;
    uint8_t             _SPSR;
    uint8_t             _SPCRO;
    uint8_t             _SPSRO;
    #endif

};

#define MII_MODEM_CLASS MiiRFClass

#endif