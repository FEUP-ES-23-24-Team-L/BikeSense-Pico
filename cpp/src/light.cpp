#include "light.h"
#include "Wire.h"
#include "sensorReading.h"

void LightSensor::setup() {
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);

  int maxAttempts = 5;
  while (maxAttempts > 0) {
    if (SI1145.Begin()) {
      initialized = true;
      break;
    }

    maxAttempts--;
    delay(500);
  }

  if (!initialized)
    Serial.println("Failed to initialize SI1145 light sensor");
}

SensorReading LightSensor::read() {
  if (!initialized) {
    return SensorReading();
  }

  return SensorReading()
      .addMeasurement("luminosity", SI1145.ReadVisible()) // Visible light in lm
      .addMeasurement("uv_level", (float)SI1145.ReadUV() / 100);
}
