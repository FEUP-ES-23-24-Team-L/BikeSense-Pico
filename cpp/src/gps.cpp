#include "gps.h"
#include "sensorReading.h"

#include <Arduino.h>
#include <iomanip>
#include <sstream>

// #define GPS_DEBUG

void Gps::setup() { Serial1.begin(9600); }

void Gps::update() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      for (int i = 0; i < this->bufferIndex_; i++) {
#ifdef GPS_DEBUG: 
        Serial.write(this->buffer_[i]);
#endif
        this->gps_.encode(this->buffer_[i]);
      }
      this->bufferIndex_ = 0;
#ifdef GPS_DEBUG:
      Serial.println();
#endif
      break;
    }
    this->buffer_[bufferIndex_] = c;
    this->bufferIndex_++;
  }
}

bool Gps::isValid() {
  return this->gps_.location.isValid() && this->gps_.altitude.isValid() &&
         this->gps_.time.isValid() && this->gps_.date.isValid();
}

bool Gps::isUpdated() { return this->gps_.location.isUpdated(); }

bool Gps::isOld() {
  return this->gps_.location.age() > this->MAX_READING_AGE_MS ||
         this->gps_.altitude.age() > this->MAX_READING_AGE_MS ||
         this->gps_.time.age() > this->MAX_READING_AGE_MS ||
         this->gps_.date.age() > this->MAX_READING_AGE_MS;
}

SensorReading Gps::read() {
  SensorReading gpsRead =
      SensorReading()
          .addMeasurement("latitude", this->gps_.location.lat())
          .addMeasurement("longitude", this->gps_.location.lng())
          .addMeasurement("altitude", this->gps_.altitude.meters());

  if (this->gps_.speed.isValid()) {
    gpsRead.addMeasurement("speed", this->gps_.speed.kmph());
  }
  if (this->gps_.course.isValid()) {
    gpsRead.addMeasurement("course", this->gps_.course.deg());
  }
  if (this->gps_.satellites.isValid()) {
    gpsRead.addMeasurement("satellites_in_use", this->gps_.satellites.value());
  }
  if (this->gps_.hdop.isValid()) {
    gpsRead.addMeasurement("hdop", this->gps_.hdop.hdop());
  }

  return gpsRead;
}

std::string Gps::timeString() {
  uint16_t year = this->gps_.date.year();
  uint8_t month = this->gps_.date.month();
  uint8_t day = this->gps_.date.day();
  uint8_t hour = this->gps_.time.hour();
  uint8_t minute = this->gps_.time.minute();
  uint8_t second = this->gps_.time.second();

  // Construct a std::tm structure
  std::tm datetime = {};
  datetime.tm_year = year - 1900; // years since 1900
  datetime.tm_mon = month;        // months since January
  datetime.tm_mday = day;         // day of the month
  datetime.tm_hour = hour;        // hours since midnight
  datetime.tm_min = minute;       // minutes after the hour
  datetime.tm_sec = second;       // seconds after the minute

  // Convert std::tm to std::time_t
  std::time_t time = std::mktime(&datetime);

  // Convert std::time_t to ISO 8601 format
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");

  return oss.str();
}
