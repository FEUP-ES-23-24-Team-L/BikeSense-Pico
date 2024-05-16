#ifndef _INFO_LED_
#define _INFO_LED_

#include "ChainableLED.h"

class InfoLed {
  const int CLOCK_PIN = 6;
  const int DATA_PIN = 7;
  const int N_LEDS = 1;

  bool isOn = false;

  ChainableLED leds = ChainableLED(CLOCK_PIN, DATA_PIN, N_LEDS);

  void setup();
};

#endif
