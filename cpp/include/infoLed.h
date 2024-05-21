#ifndef _INFO_LED_
#define _INFO_LED_

#include "ChainableLED.h"
#include "interfaces.h"

class InfoLed : public LedInterface {
  const byte CLOCK_PIN = 6;
  const byte DATA_PIN = 7;
  const byte N_LEDS = 1;

  bool isOn = false;

  ChainableLED leds = ChainableLED(CLOCK_PIN, DATA_PIN, N_LEDS);

  void setup() override;
  void setColor(byte r, byte g, byte b) override;
};

#endif
