#include "sensorReading.h"

#include <sstream>

SensorReading::SensorReading(const std::string& sensorType)
    : sensorType_(sensorType) {
    measurements_ = std::unordered_map<std::string, double>();
}

SensorReading& SensorReading::addMeasurement(const std::string& measurementName,
                                             double value) {
    this->measurements_[measurementName] = value;
    return *this;
}

const std::string& SensorReading::getSensorType() const { return sensorType_; }

const std::unordered_map<std::string, double>& SensorReading::getMeasurements()
    const {
    return measurements_;
}

std::optional<double> SensorReading::getMeasurement(
    const std::string& measurementName) const {
    // The find method returns an iterator to the element if it is found,
    // otherwise it returns an iterator to the end of the container
    auto it = measurements_.find(measurementName);
    if (it != measurements_.end()) {
        return it->second;
    }
    return {};
}

std::string SensorReading::toString() const {
    std::string result = sensorType_ + ": ";
    for (const auto& [measurementName, value] : measurements_) {
        result += measurementName + "=" + std::to_string(value) + ", ";
    }
    return result;
}

std::string SensorReading::toJsonString() const {
    std::stringstream ss;
    ss << "{\n";
    ss << "\"sensor_type\": \"" << sensorType_ << "\",\n";
    bool isFirst = true;
    for (const auto& [measurementName, value] : measurements_) {
        if (!isFirst) {
            ss << ",\n";
        }
        isFirst = false;
        ss << "\"" << measurementName << "\": " << value;
    }
    ss << "\n}";
    return ss.str();
}
