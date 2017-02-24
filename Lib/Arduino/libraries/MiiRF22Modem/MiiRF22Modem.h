#ifndef MiiRF22Modem_h
#define MiiRF22Modem_h
/**
MiiRF22Modem is build on the RadioHead library with options is needed to do time
synchronization between 2 or devices. It also implements a own Acknowledge
system and buffering of collision messages.
It has been combined into one library because it will save 2.5kb of mem

TODO: SPLIT CODE SO IT CAN BE USED IN MiiRF also using a const to replace name

In future we should see if we can use  uint32_t getLastPreambleTime() instead of _rxTime
*/
#include <MiiGenericModem.h>

#define MII_RF_MAX_MESSAGE_LEN MII_MAX_MESSAGE_LEN

//Check if the message size is big enough to receive timing_t message ()
#if  MII_SIZEOF_timing_t > MII_RF_MAX_MESSAGE_LEN
  #error The MII_RF_MAX_MESSAGE_LEN is to small to support timing message
#endif

//-------- ORIGINAL RF22 -------------------------- START
//Setting modem config to fixed will save 600 bytes
//#define MII_RF_FIXED_CONFIG { 0x2b, 0x03, 0xf4, 0x20, 0x41, 0x89, 0x00, 0x36, 0x40, 0x0a, 0x1d, 0x80, 0x60, 0x10, 0x62, 0x2c, 0x22, 0x08 } //FSK 2, 5

// Max number of octets the RF22 Rx and Tx FIFOs can hold
#define MII_RF_FIFO_SIZE 64

// These values we set for FIFO thresholds (4, 55) are actually the same as the POR values
#define MII_RF_TXFFAEM_THRESHOLD 4
#define MII_RF_RXFFAFULL_THRESHOLD 55

// This is the default node address,
#define MII_RF_DEFAULT_NODE_ADDRESS 0

// This address in the TO addreess signifies a broadcast
#define MII_RF_BROADCAST_ADDRESS MII_BROADCAST_ADDRESS

// Number of registers to be passed to setModemConfig(). Obsolete.
#define MII_RF_NUM_MODEM_CONFIG_REGS 18

//Create a unique name for each class and set constants depending on types
#define MII_TXPOW_1DBM                         0x00
#define MII_TXPOW_2DBM                         0x01
#define MII_TXPOW_5DBM                         0x02
#define MII_TXPOW_8DBM                         0x03
#define MII_TXPOW_11DBM                        0x04
#define MII_TXPOW_14DBM                        0x05
#define MII_TXPOW_17DBM                        0x06
#define MII_TXPOW_20DBM                        0x07

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

class MiiRF22Modem : public MiiGenericModem {
public:
  ///Init the based independent of RF device used
  MiiRF22Modem(uint8_t selectPin = MII_PIN_RF_CS, uint8_t intPin = MII_PIN_RF_IRQ, uint8_t sdnPin = MII_PIN_RF_SDN);

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
    bool  init(uint8_t address,uint16_t syncWords=0x2dd4,uint8_t channel=1,uint8_t power=MII_TXPOW_17DBM,bool isMaster=false,ModemConfigChoice index=FSK_Rb2Fd5);

    /// Turns the receiver on if it not already on.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to available space in buf. Set to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
    bool internalRecv(uint8_t* buf=0, uint8_t* len=0);

    /// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
    /// Then loads a message into the transmitter and starts the transmitter. Note that a message length
    /// of 0 is NOT permitted. If the message is too long for the underlying radio technology, send() will
    /// return false and will not send the message.
    /// \param[in] data Array of data to be sent
    /// \param[in] len Number of bytes of data to send (> 0)
    /// \return true if the message length was valid and it was correctly queued for transmit
    bool send(const uint8_t* data=0, uint8_t len=0);


    /// Issues a software reset to the
    /// RF22 module. Blocks for 1ms to ensure the reset is complete.
    void           reset();

    /// Reads and returns the device status register MII_RF_REG_02_DEVICE_STATUS
    /// \return The value of the device status register
    uint8_t        statusRead();

    /// Sets the transmitter and receiver centre frequency
    /// \param[in] centre Frequency in MHz. 240.0 to 960.0. Caution, some versions of RF22 and derivatives
    /// implemented more restricted frequency ranges.
    /// \param[in] afcPullInRange Sets the AF Pull In Range in MHz. Defaults to 0.05MHz (50kHz).
    /// Range is 0.0 to 0.159375
    /// for frequencies 240.0 to 480MHz, and 0.0 to 0.318750MHz for  frequencies 480.0 to 960MHz,
    /// \return true if the selected frquency centre + (fhch * fhs) is within range and the afcPullInRange
    /// is within range
    bool        setFrequency(float centre, float width = 0.05);

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

    /// Tells the receiver to accept messages with any TO address, not just messages
    /// addressed to this node or the broadcast address
    /// \param[in] promiscuous true if you wish to receive messages with any TO address
    void           setPromiscuous(bool promiscuous);

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
    void setSyncWords(const uint16_t syncWords);

    /// Sets the CRC polynomial top be used to generare the CRC for both receive and transmit
    /// Must be called before init(), otherwise the default of CRC_16_IBM will be used.
    /// \param[in] polynomial One of RF22::CRCPolynomial choices CRC_*
    /// \return true if polynomial is a valid option for this radio.
    bool setCRCPolynomial(CRCPolynomial polynomial);

    void setGpioReversed(bool gpioReversed);


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
    void           setHeaderTo(uint8_t to);

    /// Sets the FROM header to be sent in all subsequent messages
    /// \param[in] from The new FROM header value
    void           setHeaderFrom(uint8_t from);

    /// Sets the ID header to be sent in all subsequent messages
    /// \param[in] id The new ID header value
    void           setHeaderId(uint8_t id);
    /// Sets the FLAGS header to be sent in all subsequent messages
    /// \param[in] flags The new FLAGS header value
    void          setHeaderFlags(uint8_t set, uint8_t clear = MII_ALL_FLAGS);

protected:
    uint8_t             _idleMode; // The radio mode to use when mode is MII_RF_MODE_IDLE
    uint8_t             _deviceType;

   //internalProcess allows you to do internal processing of commands during available checks
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
    /// \return false if the resulting message would exceed MII_RF_MAX_MESSAGE_LEN, else true
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

    /// Start the transmission of the contents
    /// of the Tx buffer
    void           startTransmit();

    /// ReStart the transmission of the contents
    /// of the Tx buffer after a atransmission failure
    void           restartTransmit();

    /// Low level interrupt service routine for RF22 connected to interrupt 0
    static void         isr0();

        /// Clears the transmitter buffer
    /// Internal use only
    void  clearTxBuf();


    /// Array of instances connected to interrupts 0 and 1
    static MiiRF22Modem*   _deviceForInterrupt;

    // Message confiuration options
    CRCPolynomial       _polynomial;

    // These volatile members may get changed in the interrupt service routine
    volatile uint8_t    _bufLen;
    uint8_t             _buf[MII_RF_MAX_MESSAGE_LEN];

    volatile uint8_t    _txBufSentIndex;
};


#endif