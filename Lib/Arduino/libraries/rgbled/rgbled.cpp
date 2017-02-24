#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "rgbled.h"

static const uint16_t kDefaultDelay = 5;  // milliseconds


RGBLED::RGBLED(const byte red_pin, const byte green_pin, const byte blue_pin,const byte anode)
    : red_pin_(red_pin),
      green_pin_(green_pin),
      blue_pin_(blue_pin),
      red_value_(0),
      green_value_(0),
      blue_value_(0),
      default_delay_(kDefaultDelay),
      anode_(anode)
{
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  colorIs(0x000000); //Switch off
}


RGBLED::Intensity RGBLED::channelIntensity(const Channel channel) const
{
  switch (channel) {
    case kBlue:
      return blue_value_;
      break;
    case kGreen:
      return green_value_;
      break;
    default:
    case kRed:
      return red_value_;
      break;
  }
}


void RGBLED::channelIntensityIs(const Channel channel, const Intensity value)
{
  switch (channel) {
    case kBlue:
      analogWrite(blue_pin_, anode_ ?  ~value : value);
      blue_value_ = value;
      break;
    case kGreen:
      analogWrite(green_pin_, anode_ ?  ~value : value);
      green_value_ = value;
      break;
    default:
    case kRed:
      analogWrite(red_pin_, anode_ ?  ~value : value );
      red_value_ = value;
      break;
  }
}


void RGBLED::colorIs(Intensity red, Intensity green, Intensity blue)
{
  channelIntensityIs(kRed, red);
  channelIntensityIs(kGreen, green);
  channelIntensityIs(kBlue, blue);
}


void RGBLED::colorIs(uint32_t color)
{
  Intensity red   = (color & (0xFF0000)) >> 16;
  Intensity green = (color & (0x00FF00)) >> 8;
  Intensity blue  = (color & (0x0000FF));
  colorIs(red, green, blue);
}

void RGBLED::colorOnOff(byte state,uint32_t rgbcolor,uint32_t colorCondition){
  if (!colorCondition || colorCondition!=color())
     colorIs(state ? rgbcolor : 0x000000);
}

uint32_t RGBLED::color()
{
  return (red_value_ << 16 | green_value_ << 8 | blue_value_);
}

void RGBLED::fadeChannel(const Channel channel,
                         const Intensity initial,
                         const Intensity final,
                         const uint16_t delay_ms)
{
  uint8_t mask = channel;
  fadeChannels(mask, initial, final, delay_ms);
}


void RGBLED::fadeChannels(const uint8_t channel_mask,
                          const Intensity initial,
                          const Intensity final,
                          uint16_t delay_ms)
{
  delay_ms = (delay_ms == 0) ? defaultDelay() : delay_ms;

  if (initial < final || initial == final) {
    // Fade up.
    for (Intensity i = initial; i < final; ++i) {
      writeChannels(channel_mask, i);
      delay(delay_ms);
    }
  } else {
    // Fade down.
    for (Intensity i = initial; i > final; --i) {
      writeChannels(channel_mask, i);
      delay(delay_ms);
    }
  }

  // Support final = 255 or final = 0, where ++i/--i would have overflown.
  delay(delay_ms);
  writeChannels(channel_mask, final);
}


void RGBLED::writeChannels(const uint8_t channel_mask, const Intensity value)
{
  if (channel_mask & kRed)
    channelIntensityIs(kRed, value);
  if (channel_mask & kGreen)
    channelIntensityIs(kGreen, value);
  if (channel_mask & kBlue)
    channelIntensityIs(kBlue, value);
}
