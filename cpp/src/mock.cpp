#include <Arduino.h>
#include <mock.h>
#include <sensorReading.h>

#include <random>

void MockSensor::setup() { Serial.println("Mock sensor is setting up..."); }

SensorReading MockSensor::read() {
    return SensorReading("mock all-in-one sensor")
        .addMeasurement("noise_level", 1)
        .addMeasurement("temperature", 2)
        .addMeasurement("humidity", 3)
        .addMeasurement("uv_level", 4)
        .addMeasurement("luminosity", 5)
        .addMeasurement("carbon_monoxide_level", 6)
        .addMeasurement("polution_particles_ppm", 7);
}

void MockGps::setup() { Serial.println("Mock GPS is setting up..."); }

SensorReading MockGps::read() {
    return SensorReading("mock gps")
        .addMeasurement("latitude", 8)
        .addMeasurement("longitude", 9)
        .addMeasurement("altitude", 10)
        .addMeasurement("speed", 11)
        .addMeasurement("course", 12)
        .addMeasurement("satellites_in_use", 13)
        .addMeasurement("fix_type", 14)
        .addMeasurement("hdop", 15)
        .addMeasurement("vdop", 16)
        .addMeasurement("pdop", 17);
}

void MockDataStorage::setup() {
    Serial.println("Mock data storage is setting up...");
    readings_ = std::vector<SensorReading>();
}

void MockDataStorage::store(const SensorReading& reading) {

    if (readings_.size() > 10) {
        readings_.erase(readings_.begin());
    }
    readings_.push_back(reading);
    Serial.printf("Mock data storage stored reading: %s\n", reading.toJsonString().c_str());
}

std::vector<SensorReading> MockDataStorage::retrieve(int batchSize) {
    std::vector<SensorReading> result;
    for (int i = 0; i < batchSize; i++) {
        if (readings_.size() > 0) {
            result.push_back(readings_.back());
            readings_.pop_back();
        }
    }
    return result;
}

