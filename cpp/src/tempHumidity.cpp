#include "tempHumidity.h"

void TempHumiditySensor::setup() { dht.begin(); }

SensorReading TempHumiditySensor::read() {
  SensorReading reading;

  sensors_event_t tempEvent;
  sensors_event_t humidityEvent;

  dht.temperature().getEvent(&tempEvent);
  dht.humidity().getEvent(&humidityEvent);

  if (!isnan(tempEvent.temperature)) {
    reading.addMeasurement("temperature", tempEvent.temperature);
  }

  if (!isnan(humidityEvent.relative_humidity)) {
    reading.addMeasurement("humidity", humidityEvent.relative_humidity);
  }

  return reading;
}
