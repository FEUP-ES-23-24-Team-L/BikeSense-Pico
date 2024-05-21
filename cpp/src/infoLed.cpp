#include "infoLed.h"

void InfoLed::setup() { leds.setColorRGB(0, 0, 0, 0); }
void InfoLed::setColor(byte r, byte g, byte b) { leds.setColorRGB(0, r, g, b); }
