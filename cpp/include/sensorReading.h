#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <optional>
#include <string>
#include <unordered_map>

class SensorReading {
private:
  std::unordered_map<std::string, double> measurements_;

public:
  // Constructor to initialize with sensor type and measurements
  SensorReading();
  SensorReading(std::unordered_map<std::string, double> measurements);

  SensorReading &addMeasurement(const std::string &measurementName,
                                double value);

  // Getter functions all measurements
  const std::unordered_map<std::string, double> &getMeasurements() const;

  // Getter function for a specific measurement
  std::optional<double>
  getMeasurement(const std::string &measurementName) const;

  // Overloaded operators for merging sensor readings
  SensorReading operator+(const SensorReading &other) const;
  SensorReading &operator+=(const SensorReading &other);
};

#endif
