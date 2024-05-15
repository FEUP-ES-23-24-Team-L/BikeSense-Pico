#include "bikesense.h"
#include "elapsedMillis.h"
#include "interfaces.h"

#include <Arduino.h>
#include <ArduinoJson.h>

BikeSenseBuilder::BikeSenseBuilder() {
  sensors_ = std::vector<SensorInterface *>();
  gps_ = nullptr;
  dataStorage_ = nullptr;
}

BikeSenseBuilder &BikeSenseBuilder::addSensor(SensorInterface *sensor) {
  sensors_.push_back(sensor);
  return *this;
}

BikeSenseBuilder &BikeSenseBuilder::addGps(GpsInterface *gps) {
  gps_ = gps;
  return *this;
}

BikeSenseBuilder &
BikeSenseBuilder::addDataStorage(DataStorageInterface *dataStorage) {
  dataStorage_ = dataStorage;
  return *this;
}

BikeSenseBuilder &BikeSenseBuilder::whoAmI(const std::string bikeCode,
                                           const std::string unitCode) {
  bikeCode_ = bikeCode;
  unitCode_ = unitCode;
  return *this;
}

BikeSenseBuilder &
BikeSenseBuilder::withApiConfig(const std::string &apiAuthToken,
                                const std::string &apiEndpoint) {
  apiAuthToken_ = apiAuthToken;
  apiEndpoint_ = apiEndpoint;
  return *this;
}

BikeSenseBuilder &BikeSenseBuilder::addNetwork(const std::string &ssid,
                                               const std::string &password) {
  this->networks_[ssid] = password;
  return *this;
}

BikeSense BikeSenseBuilder::build() {
  return BikeSense(sensors_, gps_, dataStorage_, networks_, bikeCode_,
                   unitCode_, apiAuthToken_, apiEndpoint_);
}

BikeSense::BikeSense(std::vector<SensorInterface *> sensors, GpsInterface *gps,
                     DataStorageInterface *dataStorage,
                     const StringMap &networks, const std::string &bikeCode,
                     const std::string &unitCode,
                     const std::string &apiAuthToken,
                     const std::string &apiEndpoint, const WiFiMode_t wifi_mode,
                     const int sensor_read_interval_ms,
                     const int wifi_retry_interval_ms,
                     const int http_timeout_ms, const int upload_batch_size)
    : sensors_(sensors), gps_(gps), dataStorage_(dataStorage),
      SENSOR_READ_INTERVAL_MS(sensor_read_interval_ms),
      WIFI_RETRY_INTERVAL_MS(wifi_retry_interval_ms),
      HTTP_TIMEOUT_MS(http_timeout_ms), UPLOAD_BATCH_SIZE(upload_batch_size),
      API_TOKEN(apiAuthToken), API_ENDPOINT(apiEndpoint), BIKE_CODE(bikeCode),
      UNIT_CODE(unitCode) {

  WiFi.mode(wifi_mode);
  for (const auto &[ssid, password] : networks) {
    multi_.addAP(ssid.c_str(), password.c_str());
  }

  http_.setReuse(true);
  http_.setInsecure();
  http_.setTimeout(HTTP_TIMEOUT_MS);
}

void BikeSense::setupSensors() {
  gps_->setup();
  if (!dataStorage_->setup()) {
    Serial.println("Failed to setup data storage");
    this->state_ = ERROR;
  }

  for (auto sensor : sensors_) {
    sensor->setup();
  }
}

SensorReading BikeSense::readSensors() const {
  SensorReading readings;

  readings += (gps_->read());
  for (auto sensor : sensors_) {
    readings += (sensor->read());
  }

  return readings;
}

inline bool BikeSense::checkWifi() { return multi_.run() == WL_CONNECTED; }

int BikeSense::registerAndGetID(std::string payload, std::string endpoint) {

  http_.begin((API_ENDPOINT + endpoint).c_str());
  http_.addHeader("Content-Type", "application/json");
  http_.addHeader("Authorization", API_TOKEN.c_str());

  int httpCode = http_.POST(payload.c_str());
  if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
    Serial.printf("Failed to register parameter with error %d (%s)\n", httpCode,
                  http_.errorToString(httpCode).c_str());
    return -1;
  }

  const String &response = http_.getString();
  Serial.printf("Response: %s\n", response.c_str());
  JsonDocument doc;
  deserializeJson(doc, response);
  http_.end();

  return doc["id"];
}

int BikeSense::registerTripAndGetID() {

  if (!registered_) {
    std::string bikeCodePayload = "{\"code\": \"" + BIKE_CODE + "\"}";
    std::string unitCodePayload = "{\"code\": \"" + UNIT_CODE + "\"}";
    bikeId_ = registerAndGetID(bikeCodePayload, "/bike/register");
    unitId_ = registerAndGetID(unitCodePayload, "/sensor_unit/register");
    if (bikeId_ == -1 || unitId_ == -1) {
      return -1;
    }
    registered_ = true;
  }

  std::string tripPayload = "{\"bike_id\": " + std::to_string(bikeId_) +
                            ", \"sensor_unit_id\": " + std::to_string(unitId_) +
                            "}";

  return registerAndGetID(tripPayload, "/trip/register");
}

int BikeSense::uploadData(const std::vector<std::string> &readings) {
  JsonDocument doc;
  std::string payload;

  Serial.printf("Uploading %d readings:\n", readings.size());
  for (size_t i = 0; i < readings.size(); i++) {
    doc[i] = readings[i];
  }
  serializeJson(doc, payload);
  Serial.println(payload.c_str());

  return http_.POST(payload.c_str());
}

int BikeSense::saveData(const SensorReading rd, const std::string timestamp) {
  JsonDocument doc;
  std::string json;

  doc["timestamp"] = timestamp;
  for (auto meas : rd.getMeasurements()) {
    doc[meas.first] = meas.second;
  }
  serializeJson(doc, json);
  this->dataStorage_->store(json);

  return 0;
}

bool BikeSense::uploadAllSensorData() {
  int tripId_ = registerTripAndGetID();
  if (tripId_ == -1) {
    Serial.println("Failed to register trip");
    http_.end();
    return false;
  }

  http_.begin((API_ENDPOINT + "/trip/upload_data").c_str());
  http_.addHeader("Content-Type", "application/json");
  http_.addHeader("Authorization", API_TOKEN.c_str());
  http_.addHeader("Trip-ID", String(tripId_).c_str());

  Serial.println("Uploading all sensor data...");

  int sucessfullUploads = 0;
  retrievedData data;
  while (data = dataStorage_->retrieve(UPLOAD_BATCH_SIZE), data.has_value()) {
    int httpCode = uploadData(data.value());
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
      Serial.printf("HTTP post failed after %d batches with error %s\n",
                    sucessfullUploads, http_.errorToString(httpCode).c_str());
      for (auto reading : data.value()) {
        dataStorage_->store(reading);
      }
      http_.end();
      return false;
    }
    sucessfullUploads++;
  }

  http_.end();
  return true;
}

void BikeSense::run() {
  this->setupSensors();

  elapsedMillis wifi_retry_timer_ = WIFI_RETRY_INTERVAL_MS;

  // TODO: clean up the loop state machine
  while (true) {
    switch (this->state_) {

    case IDLE: {
      Serial.println("Checking for known wifi connections");
      if (!this->checkWifi()) {
        this->state_ = COLLECTING_DATA;
      }
    } break;

    case COLLECTING_DATA: {

      this->gps_->update();
      if (!this->gps_->isValid()) {
        Serial.println("GPS data is invalid, skipping sensor readings");
        // TODO: check if this can sleep for more time
        break;
      }

      SensorReading readings = this->readSensors();
      this->saveData(readings, this->gps_->timeString());

      if (wifi_retry_timer_ < WIFI_RETRY_INTERVAL_MS) {
        break;
      }
      wifi_retry_timer_ = 0;

      Serial.println("Checking for known wifi connections");
      if (this->checkWifi()) {
        this->state_ = UPLOADING_DATA;
      } else {
        Serial.printf("Wifi offline, retrying in %ds\n",
                      WIFI_RETRY_INTERVAL_MS / 1000);
      }

    } break;

    case UPLOADING_DATA: {
      Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());

      int success = uploadAllSensorData();
      if (success) {
        Serial.println("All data uploaded successfully");
        this->state_ = IDLE;
        this->dataStorage_->clear();
      } else {
        Serial.println("Error uploading data");
        this->state_ = ERROR;
      }
    } break;

    // TODO: handle this better
    case ERROR: {
      Serial.println("Error. Rebooting");
      rp2040.reboot();
    } break;
    }

    sleep_ms(1);
  }
}

// TODO: - Implement a log file to store error/log messages
//       - Implement a logger class to handle log messages
//       - Store relevant data in order to do automatic
//         upload retries for failed uploads for a certain
//         trip.
//       - Dump data to backup storage in case of failed
//         uploads and retry later (when in idle mode i.e.)
//       - Implement a watchdog timer to reboot the device (?)
//       - Brainstorm about improving trip start/end detection
//       - Automatically dump log messages to Serial when available
