#include "MiiConfig.h"


//The real config stuct
miiConfig_t MiiConfig;

uint8_t readConfig(uint32_t id,uint16_t address,uint8_t power,uint16_t firmware) {
  EEPROM_readAnything(MII_CONFIGLOCATION, MiiConfig);

  //When MII_ADDRESS is set whe will check if we need to re configure the device
  if ((address && MiiConfig.address!=address) ||
      (id && MiiConfig.id!=id) ||
      (firmware && MiiConfig.firmware!=firmware) ||
      (power && MiiConfig.power!=power)) {        //Only reconfigure device if main items changed
   //Reconfigure the bluetooth device to give better speed
    MiiConfig.firmware=firmware;
    MiiConfig.id=id;          //The serial id of the device, fixed for allways
    MiiConfig.address=address;  //The device address used
    MiiConfig.group=id % 1000;                   //The id used to group devices together
    MiiConfig.channel=id % MII_RF_CHANNEL_COUNT;                   //The device freqency used
    MiiConfig.power=power;                        //The normal power level to be used during transmission by default
    MiiConfig.timeout=750;                           //The milliseconds time between two switches, default 0,75 second
    MiiConfig.armSound=true;                               //Should we play arm sound when not armed
    MiiConfig.startDelay=2500;                             //Default start delay is 5 seconds
    MiiConfig.minBoundary=0;                               //We will not do boundary checking by default
    MiiConfig.maxBoundary=0;                               //We will not do boundary checking by default

    EEPROM_writeAnything(MII_CONFIGLOCATION, MiiConfig);
    return 1;
  }
  return 0;
}

void writeConfig(miiConfig_ptr config,uint8_t pos){
  EEPROM_writeAnything(MII_CONFIGLOCATION,pos,*config); //Dont write id(4),firmware(2),address(1)
}