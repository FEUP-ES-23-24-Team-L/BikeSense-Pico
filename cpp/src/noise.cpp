#include <Arduino.h>
#include <noise.h>
#include <optional>
#include <sensorReading.h>


void NoiseSensor::setup() { Serial.println("Mock sensor is setting up..."); }

SensorReading NoiseSensor::read() const {
  return SensorReading()
      .addMeasurement("noise_level", 77);
}