#include "pico/time.h"
#include <Arduino.h>
#include <noise.h>
#include <sensorReading.h>

void NoiseSensor::setup() { pinMode(PIN, INPUT); }

SensorReading NoiseSensor::read() {
  for (int i = 0; i < mvAvgWindowSize; i++) {
    mvAvgAccumulator += analogRead(PIN);
    sleep_us(250);
  }

  int adcReading = mvAvgAccumulator / mvAvgWindowSize - ADC_BIAS;
  int deltaDB = 20 * log10(adcReading / noiseADCReference);
  int noiseDB = noiseDBReference + deltaDB;

  return SensorReading().addMeasurement("noise_level", noiseDB);
}
