#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <string>
#include <unordered_map>
#include <optional>

class SensorReading {
   public:
    // Constructor to initialize with sensor type and measurements
    SensorReading(const std::string& sensorType);

    SensorReading& addMeasurement(const std::string& measurementName, double value);

    // Getter functions for sensor type and all measurements
    const std::string& getSensorType() const;
    const std::unordered_map<std::string, double>& getMeasurements() const;

    // Getter function for a specific measurement
    std::optional<double> getMeasurement(const std::string& measurementName) const;

    // Function to convert the sensor reading to a string
    std::string toString() const;
    std::string toJsonString() const;

   private:
    std::string sensorType_;
    std::unordered_map<std::string, double> measurements_;
};

#endif