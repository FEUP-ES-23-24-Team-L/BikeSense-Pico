#include "sensorReading.h"

SensorReading::SensorReading() {
  measurements_ = std::unordered_map<std::string, double>();
}

SensorReading::SensorReading(
    std::unordered_map<std::string, double> measurements) {
  measurements_ = measurements;
}

SensorReading &SensorReading::addMeasurement(const std::string &measurementName,
                                             double value) {
  this->measurements_[measurementName] = value;
  return *this;
}

const std::unordered_map<std::string, double> &
SensorReading::getMeasurements() const {
  return this->measurements_;
}

std::optional<double>
SensorReading::getMeasurement(const std::string &measurementName) const {
  // The find method returns an iterator to the element if it is found,
  // otherwise it returns an iterator to the end of the container
  auto it = measurements_.find(measurementName);
  if (it != measurements_.end()) {
    return it->second;
  }
  return std::nullopt;
}

SensorReading SensorReading::operator+(const SensorReading &other) const {
  std::unordered_map<std::string, double> merged_measurements(measurements_);
  // Insert all measurements from other, overwriting if keys exist
  for (const auto &[measurementName, value] : other.measurements_) {
    merged_measurements[measurementName] = value;
  }
  return SensorReading(merged_measurements);
}

SensorReading &SensorReading::operator+=(const SensorReading &other) {
  // Insert all measurements from other, overwriting if keys exist
  for (const auto &[measurementName, value] : other.measurements_) {
    measurements_[measurementName] = value;
  }
  return *this;
}
