#include "interfaces.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <bikesense.h>

BikeSenseBuilder::BikeSenseBuilder() {
  sensors_ = std::vector<SensorInterface *>();
  gps_ = nullptr;
  dataStorage_ = nullptr;
}

BikeSenseBuilder &BikeSenseBuilder::addSensor(SensorInterface *sensor) {
  sensors_.push_back(sensor);
  return *this;
}

BikeSenseBuilder &BikeSenseBuilder::addGps(SensorInterface *gps) {
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

BikeSense::BikeSense(std::vector<SensorInterface *> sensors,
                     SensorInterface *gps, DataStorageInterface *dataStorage,
                     const StringMap &networks, const std::string &bikeCode,
                     const std::string &unitCode,
                     const std::string &apiAuthToken,
                     const std::string &apiEndpoint, const WiFiMode_t wifi_mode,
                     const int sensor_read_interval_ms,
                     const int wifi_retry_interval_ms,
                     const int http_timeout_ms, const int upload_batch_size)
    : sensors_(sensors), gps_(gps), dataStorage_(dataStorage),
      wifi_retry_timer_(0), SENSOR_READ_INTERVAL_MS(sensor_read_interval_ms),
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
  dataStorage_->setup();

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

int BikeSense::uploadData(const std::vector<SensorReading> &readings) {
  std::string payload = SensorReading::toJsonArray(readings);
  Serial.printf("Uploading %d readings:\n", readings.size());
  Serial.println(payload.c_str());
  int httpCode = http_.POST(payload.c_str());
  return httpCode;
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
  // Setup the sensors
  this->setupSensors();

  wifi_retry_timer_ = WIFI_RETRY_INTERVAL_MS;

  bool collectionMode = true;
  bool dataUploaded = false;
  bool errorMode = false;

  // TODO: clean up the loop state machine
  while (true) {
    // Read the sensors
    if (collectionMode) {
      SensorReading readings = this->readSensors();
      dataStorage_->store(readings);

      // Check the WiFi connection
      if (wifi_retry_timer_ > WIFI_RETRY_INTERVAL_MS) {
        if (this->checkWifi()) {
          collectionMode = false;
          Serial.printf("Connected to WiFi: %s\n", WiFi.SSID().c_str());
        }

        wifi_retry_timer_ = 0;
      } else {
        Serial.printf("WiFi is offline, retrying in %lus\n",
                      (WIFI_RETRY_INTERVAL_MS - wifi_retry_timer_) / 1000);
      }
    } else {
      if (!dataUploaded) {
        errorMode = !uploadAllSensorData();
        dataUploaded = true;
        if (!errorMode) {
          Serial.println("Data uploaded successfully");
        } else {
          Serial.println("Failed to upload data");
          // TODO: go into warning mode (e.g. LED blinking)
          //       Block until the next retry
          //       If all retries fail, go into error mode (e.g. LED blinking
          //       faster)
        }
      }

      if (!checkWifi()) {
        collectionMode = true;
        dataUploaded = false;
        errorMode = false;
      }
    }

    sleep_ms(SENSOR_READ_INTERVAL_MS);
  }
}
