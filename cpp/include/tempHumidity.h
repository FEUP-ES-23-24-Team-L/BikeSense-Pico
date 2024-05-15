#ifndef _TEMP_HUMIDITY_H_
#define _TEMP_HUMIDITY_H_

#include "interfaces.h"
#include "sensorReading.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

class TempHumiditySensor : public SensorInterface {
  const int DHT_PIN = 22;
  const int DHT_TYPE = DHT22;

  DHT_Unified dht = DHT_Unified(DHT_PIN, DHT_TYPE);

  void setup();
  SensorReading read();
};

#endif
