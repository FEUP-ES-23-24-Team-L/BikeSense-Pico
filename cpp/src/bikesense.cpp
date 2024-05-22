#include "bikesense.h"
#include "elapsedMillis.h"
#include "interfaces.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

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

BikeSenseBuilder &BikeSenseBuilder::addLed(LedInterface *led) {
  led_ = led;
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
  return BikeSense(sensors_, gps_, dataStorage_, led_, networks_, bikeCode_,
                   unitCode_, apiAuthToken_, apiEndpoint_);
}

BikeSense::BikeSense(std::vector<SensorInterface *> sensors, GpsInterface *gps,
                     DataStorageInterface *dataStorage, LedInterface *led,
                     const StringMap &networks, const std::string &bikeCode,
                     const std::string &unitCode,
                     const std::string &apiAuthToken,
                     const std::string &apiEndpoint, const WiFiMode_t wifi_mode,
                     const int sensor_read_interval_ms,
                     const int wifi_retry_interval_ms,
                     const int http_timeout_ms, const int upload_batch_size)
    : sensors_(sensors), gps_(gps), dataStorage_(dataStorage), led_(led),
      SENSOR_READ_INTERVAL_MS(sensor_read_interval_ms),
      WIFI_RETRY_INTERVAL_MS(wifi_retry_interval_ms),
      HTTP_TIMEOUT_MS(http_timeout_ms), UPLOAD_BATCH_SIZE(upload_batch_size),
      API_TOKEN(apiAuthToken), API_ENDPOINT(apiEndpoint), BIKE_CODE(bikeCode),
      UNIT_CODE(unitCode) {

  WiFi.mode(wifi_mode);
  for (const auto &[ssid, password] : networks) {
    multi_.addAP(ssid.c_str(), password.c_str());
  }

  http_.setInsecure();
}

void BikeSense::setup() {
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

  const int nRetries = 5;

  dataStorage_->logInfo("Posting " + payload + " to " + endpoint);

  int httpCode;
  for (int rt = 0; rt < nRetries; rt++) {
    httpCode = http_.POST(payload.c_str());
    std::string msg = "Post code: " + std::to_string(httpCode) + " (attempt " +
                      std::to_string(rt) + ")";
    dataStorage_->logInfo(msg);
    if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_OK)
      break;
  }

  if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
    std::string errorMsg = "Failed to register " + endpoint;
    dataStorage_->logError(errorMsg);

    std::string httpError = http_.errorToString(httpCode).c_str();
    dataStorage_->logError(httpError);

    const String msg = http_.getString();
    if (msg != nullptr)
      dataStorage_->logError(msg.c_str());

    http_.end();
    return -1;
  }

  const String &response = http_.getString();
  JsonDocument doc;
  deserializeJson(doc, response);
  http_.end();

  return doc["id"];
}

int BikeSense::registerTripAndGetID() {
  if (!registered_) {
    dataStorage_->logInfo("Trying to register bike and sensor unit");
    std::string bikeCodePayload = "{\"code\": \"" + BIKE_CODE + "\"}";
    bikeId_ = registerAndGetID(bikeCodePayload, "/bike/register");
    std::string unitCodePayload = "{\"code\": \"" + UNIT_CODE + "\"}";
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
  std::string payload;
  payload = "[";

  for (size_t i = 0; i < readings.size() - 1; i++) {
    payload += readings[i] + ",";
  }

  payload += readings[readings.size() - 1] + "]";
  return http_.POST(payload.c_str());
}

int BikeSense::saveData(const SensorReading sensorData,
                        const SensorReading gpsData,
                        const std::string timestamp) {
  JsonDocument doc;
  doc["timestamp"] = timestamp;
  for (auto gps : gpsData.getMeasurements()) {
    doc["gps_data"][gps.first] = gps.second;
  }
  for (auto meas : sensorData.getMeasurements()) {
    doc[meas.first] = meas.second;
  }

  std::string json;
  serializeJson(doc, json);
  dataStorage_->store(json);

  return 0;
}

bool BikeSense::uploadAllSensorData() {
  dataStorage_->logInfo("Sleeping for wifi shenanigans");
  sleep_ms(3000);

  http_.begin((API_ENDPOINT + "/check_health").c_str());
  int httpCode = http_.GET();
  dataStorage_->logInfo(http_.errorToString(httpCode).c_str());
  http_.end();

  dataStorage_->logInfo("Trying to register trip");
  int tripId_ = registerTripAndGetID();
  if (tripId_ == -1) {
    dataStorage_->logError("Failed to register trip, aborting upload");
    http_.end();
    return false;
  }
  std::string msg = "Trip registered with id: " + std::to_string(tripId_);
  dataStorage_->logInfo(msg);

  dataStorage_->logInfo("Starting Bulk Data Upload");
  int nUploads = 1;
  retrievedData data;
  while (data = dataStorage_->retrieve(UPLOAD_BATCH_SIZE), data.has_value()) {
    http_.begin((API_ENDPOINT + "/trip/upload_data").c_str());
    http_.addHeader("Content-Type", "application/json");
    http_.addHeader("Authorization", API_TOKEN.c_str());
    http_.addHeader("Trip-ID", String(tripId_).c_str());

    int httpCode = uploadData(data.value());
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {

      std::string errorMsg =
          "HTTP post failed after " + std::to_string(nUploads) + " batches";
      std::string httpError = http_.errorToString(httpCode).c_str();
      std::string response = http_.getString().c_str();
      dataStorage_->logError(errorMsg);
      dataStorage_->logError(httpError);
      dataStorage_->logError(response);

      http_.end();
      return false;
    }
    dataStorage_->logInfo("Batch " + std::to_string(nUploads) + " uploaded");
    http_.end();
    nUploads++;
  }

  return true;
}

void BikeSense::run() {
  setup();

  elapsedMillis wifi_retry_timer_ = WIFI_RETRY_INTERVAL_MS;
  elapsedMillis builtin_led_timer_ = 0;

  const int LED_BLINK_INTERVAL_MS = 500;
  bool builtin_led_state = false;

  bool logsDumped = false;

  while (true) {
    switch (state_) {

    case IDLE: {
      led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, led_->BYTE_MAX);
      if (!checkWifi()) {
        state_ = COLLECTING_DATA;
        dataStorage_->logInfo("Starting data collection for new trip");
      }
    } break;

    case COLLECTING_DATA: {
      gps_->update();

      if (gps_->isValid() && !gps_->isOld()) {
        led_->setColor(0, led_->BYTE_MAX, 0);
        // Wait for the GPS data to be updated
        if (gps_->isUpdated()) {
          saveData(readSensors(), gps_->read(), gps_->timeString());
        }
      } else {
        led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, 0);
        // Serial.println("GPS data is invalid, skipping sensor readings");
      }

      if (wifi_retry_timer_ < WIFI_RETRY_INTERVAL_MS) {
        break;
      }
      wifi_retry_timer_ = 0;

      Serial.println("Checking for known wifi connections");
      if (checkWifi())
        state_ = UPLOADING_DATA;

      Serial.printf("Wifi offline, retrying in %ds\n",
                    WIFI_RETRY_INTERVAL_MS / 1000);

    } break;

    case UPLOADING_DATA: {
      led_->setColor(0, 0, led_->BYTE_MAX);

      std::string wifiMsg =
          "Connected to WiFi: " + std::string(WiFi.SSID().c_str());
      std::string endpointMsg = "Uploading data to " + API_ENDPOINT;
      dataStorage_->logInfo(wifiMsg);
      dataStorage_->logInfo(endpointMsg);

      int success = uploadAllSensorData();
      if (success) {
        state_ = IDLE;
        dataStorage_->logInfo("Data upload successful, clearing storage");
        dataStorage_->clear();
      } else {
        dataStorage_->logError("Failed to upload data, going into error mode");
        state_ = ERROR;
      }
    } break;

    // TODO: handle this better
    case ERROR: {
      led_->setColor(led_->BYTE_MAX, 0, 0);
      dataStorage_->logError("Rebooting device due to error...");
      rp2040.reboot();
    } break;
    }

    // NOTE: Blink the builtin LED as a heartbeat indicator
    if (builtin_led_timer_ > LED_BLINK_INTERVAL_MS) {
      builtin_led_timer_ = 0;
      builtin_led_state = !builtin_led_state;
      digitalWrite(LED_BUILTIN, builtin_led_state);
    }

    sleep_ms(1);
  }
}

// TODO: - Store relevant data in order to do automatic
//         upload retries for failed uploads for a certain
//         trip.
//       - Dump data to backup storage in case of failed
//         uploads and retry later (when in idle mode i.e.)
//       - Implement a watchdog timer to reboot the device (?)
//       - Brainstorm about improving trip start/end detection
