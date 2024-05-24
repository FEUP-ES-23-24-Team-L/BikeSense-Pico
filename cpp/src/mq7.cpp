#include "mq7.h"

void MQ7GasSensor::setup() {
  cycleTimer = 0;
  isHeatCycle = true;
  analogWrite(MQ7_PIN, 255);
}

// NOTE: This sensor requires manual calibration and will not be precise.
// Should be used as a proof of concept only and be swapped out for a more
// robust sensor in a production environment.
SensorReading MQ7GasSensor::read() {
  if (cycleTimer > (isHeatCycle ? HEAT_CYCLE_TIME : COOL_CYCLE_TIME)) {
    cycleTimer = 0;
    isHeatCycle = !isHeatCycle;
    analogWrite(MQ7_PIN, isHeatCycle ? 255.0 : 72);

    if (isHeatCycle) {
      sleep_ms(10);
      int sensorValue = analogRead(MQ7_PIN);
      return SensorReading().addMeasurement("carbon_monoxide_level",
                                            sensorValue);
    }
  }

  return SensorReading();
}
