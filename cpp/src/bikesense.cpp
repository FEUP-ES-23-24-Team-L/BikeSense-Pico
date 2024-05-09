#include <Arduino.h>
#include <bikesense.h>

BikeSenseBuilder::BikeSenseBuilder() {
    BIKE_ID = 0;
    UNIT_ID = 0;

    sensors_ = std::vector<SensorInterface*>();
    gps_ = nullptr;
    dataStorage_ = nullptr;
}

BikeSenseBuilder& BikeSenseBuilder::addSensor(SensorInterface* sensor) {
    sensors_.push_back(sensor);
    return *this;
}

BikeSenseBuilder& BikeSenseBuilder::addGps(SensorInterface* gps) {
    gps_ = gps;
    return *this;
}

BikeSenseBuilder& BikeSenseBuilder::addDataStorage(
    DataStorageInterface* dataStorage) {
    dataStorage_ = dataStorage;
    return *this;
}

BikeSenseBuilder& BikeSenseBuilder::whoAmI(const int bikeId, const int unitId) {
    BIKE_ID = bikeId;
    UNIT_ID = unitId;
    return *this;
}

BikeSenseBuilder& BikeSenseBuilder::withApiConfig(
    const std::string& apiToken, const std::string& apiEndpoint) {
    API_TOKEN = apiToken;
    API_ENDPOINT = apiEndpoint;
    return *this;
}

BikeSenseBuilder& BikeSenseBuilder::withWifiConfig(
    const std::string& ssid, const std::string& password) {
    WIFI_SSID = ssid;
    WIFI_PASSWORD = password;
    return *this;
}

BikeSense BikeSenseBuilder::build() {
    return BikeSense(sensors_, gps_, dataStorage_, BIKE_ID, UNIT_ID, API_TOKEN,
                     API_ENDPOINT, WIFI_SSID, WIFI_PASSWORD);
}

BikeSense::BikeSense(std::vector<SensorInterface*> sensors,
                     SensorInterface* gps, DataStorageInterface* dataStorage,
                     int bikeId, int unitId, const std::string& apiToken,
                     const std::string& apiEndpoint, const std::string& ssid,
                     const std::string& password)
    : sensors_(sensors),
      gps_(gps),
      dataStorage_(dataStorage),
      BIKE_ID(bikeId),
      UNIT_ID(unitId),
      API_TOKEN(apiToken),
      API_ENDPOINT(apiEndpoint),
      WIFI_SSID(ssid),
      WIFI_PASSWORD(password) {}

void BikeSense::setupSensors() {
    gps_->setup();
    dataStorage_->setup();

    for (auto sensor : sensors_) {
        sensor->setup();
    }
}

std::vector<SensorReading> BikeSense::readSensors() {
    std::vector<SensorReading> readings;

    readings.push_back(gps_->read());
    for (auto sensor : sensors_) {
        readings.push_back(sensor->read());
    }

    return readings;
}

void BikeSense::run() {
    // Setup the sensors
    this->setupSensors();

    while (true) {
        // Read the sensors
        std::vector<SensorReading> readings = this->readSensors();

        // Store the readings
        for (auto reading : readings) dataStorage_->store(reading);

        sleep_ms(1000);
    }
}
