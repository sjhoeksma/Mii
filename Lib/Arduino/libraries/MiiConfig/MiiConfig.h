#ifndef MiiConfig_h
#define MiiConfig_h
//MiiConfig is implementation config settings
#include <Mii.h>
#include <EEPROM.h>            //The Basic EEPRom lib
#include <EEPROMAnything.h>    //Read write blocks from eeprom

extern miiConfig_t MiiConfig;
uint8_t readConfig(uint32_t id,uint16_t address,uint8_t power,uint16_t firmware=100);
void writeConfig(miiConfig_ptr config,uint8_t pos=7);

#endif