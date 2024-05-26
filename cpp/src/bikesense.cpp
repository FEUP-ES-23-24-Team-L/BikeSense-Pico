#include "bikesense.h"
#include "elapsedMillis.h"
#include "interfaces.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <optional>
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

  if (led_ != nullptr) {
    led_->setup();
  }

  for (auto sensor : sensors_) {
    sensor->setup();
  }

  dataStorage_->logInfo("BikeSense setup complete", gps_->timeString());
}

SensorReading BikeSense::readSensors() const {
  SensorReading readings;

  for (auto sensor : sensors_) {
    readings += (sensor->read());
  }

  return readings;
}

int BikeSense::registerAndGetID(std::string payload, std::string endpoint) {
  http_.begin((API_ENDPOINT + endpoint).c_str());
  http_.addHeader("Content-Type", "application/json");
  http_.addHeader("Authorization", API_TOKEN.c_str());

  const int nRetries = 5;

  dataStorage_->logInfo("Posting " + payload + " to " + endpoint,
                        gps_->timeString());

  int httpCode;
  for (int rt = 0; rt < nRetries; rt++) {
    httpCode = http_.POST(payload.c_str());
    std::string msg = "Post code: " + std::to_string(httpCode) + " (attempt " +
                      std::to_string(rt) + ")";
    dataStorage_->logInfo(msg, gps_->timeString());
    if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_OK)
      break;
  }

  if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
    std::string errorMsg = "Failed to register " + endpoint;
    dataStorage_->logError(errorMsg, gps_->timeString());

    std::string httpError = http_.errorToString(httpCode).c_str();
    dataStorage_->logError(httpError, gps_->timeString());

    const String msg = http_.getString();
    if (msg != nullptr)
      dataStorage_->logError(msg.c_str(), gps_->timeString());

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

bool BikeSense::uploadTrip(std::string filename, const int tripId,
                           const int startingBatchIndex) {
  int nBatches = 1;
  retrievedData data;

  dataStorage_->logInfo("Starting Bulk Data Upload", gps_->timeString());
  while (data = dataStorage_->retrieve(filename, UPLOAD_BATCH_SIZE),
         data.has_value()) {
    if (nBatches < startingBatchIndex) {
      nBatches++;
      continue;
    }

    http_.begin((API_ENDPOINT + "/trip/upload_data").c_str());
    http_.addHeader("Content-Type", "application/json");
    http_.addHeader("Authorization", API_TOKEN.c_str());
    http_.addHeader("Trip-ID", String(tripId).c_str());

    int httpCode = uploadData(data.value());
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
      std::string errorMsg =
          "HTTP post failed after " + std::to_string(nBatches) + " batches";
      std::string httpError = http_.errorToString(httpCode).c_str();
      std::string response = http_.getString().c_str();

      dataStorage_->logError(errorMsg, gps_->timeString());
      dataStorage_->logError(httpError, gps_->timeString());
      dataStorage_->logError(response, gps_->timeString());

      dataStorage_->backupFile(filename, tripId, nBatches);

      http_.end();
      return false;
    }

    dataStorage_->logInfo("Batch " + std::to_string(nBatches) + " uploaded",
                          gps_->timeString());
    http_.end();
    nBatches++;
  }

  return true;
}

bool BikeSense::registerAndUploadTrip(std::string filename) {
  // HACK: for some reason, the first request always fails, this is a workaround
  {
    http_.begin((API_ENDPOINT + "/check_health").c_str());
    int httpCode = http_.GET();
    http_.end();
  }

  dataStorage_->logInfo("Trying to register trip", gps_->timeString());
  int tripId = registerTripAndGetID();
  if (tripId == -1) {
    dataStorage_->logError("Failed to register trip, aborting upload",
                           gps_->timeString());
    dataStorage_->backupFile(filename, std::nullopt, std::nullopt);
    http_.end();
    return false;
  }

  std::string msg = "Trip registered with id: " + std::to_string(tripId);
  dataStorage_->logInfo(msg, gps_->timeString());
  return uploadTrip(filename, tripId, 0);
}

void BikeSense::uploadBackups(std::vector<std::string> &backupFiles) {
  led_->setColor(led_->BYTE_MAX, 0, led_->BYTE_MAX);

  for (auto file : backupFiles) {
    dataStorage_->logInfo("Found backup file: " + std::string(file),
                          gps_->timeString());
  }

  while (backupFiles.size() > 0) {
    if (multi_.run() != WL_CONNECTED) {
      break;
    }

    std::string file = backupFiles.back();
    bool uploadSuccess = false;
    dataStorage_->logInfo("Uploading backup file: " + std::string(file),
                          gps_->timeString());

    int tripId, batchIndex;
    dataStorage_->decodeFileName(file, tripId, batchIndex);

    if (tripId != -1) {
      dataStorage_->logInfo("Resuming upload for trip id: " +
                                std::to_string(tripId),
                            gps_->timeString());

      uploadSuccess = uploadTrip(file, tripId, batchIndex);
    } else
      uploadSuccess = registerAndUploadTrip(file);

    if (!uploadSuccess) {
      dataStorage_->logError("Backup upload failed for file " +
                                 std::string(file),
                             gps_->timeString());
      break;
    }

    dataStorage_->logInfo("Backup upload successful", gps_->timeString());
    dataStorage_->clear(file);
    dataStorage_->logInfo("Backup cleared", gps_->timeString());
    backupFiles.pop_back();
  }
}

void BikeSense::run() {
  setup();

  const int MOVE_READ_INTERVAL_MS = 1000;
  const int STATIONARY_READ_INTERVAL_MS = 60000; // 1 minute

  elapsedMillis wifi_retry_timer_ = WIFI_RETRY_INTERVAL_MS;
  elapsedMillis sensor_read_timer_ = 0;
  elapsedMillis builtin_led_timer_ = 0;

  const int LED_BLINK_INTERVAL_MS = 500;
  bool builtin_led_state = false;

  auto checkWifi = [this, &wifi_retry_timer_]() {
    if (wifi_retry_timer_ < WIFI_RETRY_INTERVAL_MS)
      return false;

    wifi_retry_timer_ = 0;

    Serial.println("Checking for known wifi connections");
    if (multi_.run() != WL_CONNECTED) {

      Serial.printf("Wifi offline, retrying in %ds\n",
                    WIFI_RETRY_INTERVAL_MS / 1000);
      return false;
    }

    return true;
  };

  std::vector<std::string> backupFiles = dataStorage_->getBackUpFiles();
  led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, led_->BYTE_MAX);

  while (true) {
    gps_->update();

    // NOTE: Blink the builtin LED as a heartbeat indicator
    if (builtin_led_timer_ > LED_BLINK_INTERVAL_MS) {
      builtin_led_timer_ = 0;
      builtin_led_state = !builtin_led_state;
      digitalWrite(LED_BUILTIN, builtin_led_state);
    }

    switch (state_) {

    case IDLE: {
      if (!checkWifi()) {
        state_ = COLLECTING_DATA;
        led_->setColor(0, led_->BYTE_MAX, 0);
        dataStorage_->logInfo("Starting data collection for new trip",
                              gps_->timeString());
      }

      if (backupFiles.size() > 0) {
        uploadBackups(backupFiles);
      }

    } break;

    case COLLECTING_DATA: {
      SensorReading sensorData = readSensors();

      if (!gps_->isValid() || gps_->isOld()) {
        state_ = NO_GPS;
        led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, 0);
        dataStorage_->logInfo("GPS signal lost", gps_->timeString());
        break;
      }

      if (checkWifi()) {
        state_ = UPLOADING_DATA;
        break;
      }

      if (!gps_->isUpdated()) {
        break;
      }

      if (gps_->isMoving()) {
        led_->setColor(0, led_->BYTE_MAX, 0);
        if (sensor_read_timer_ > MOVE_READ_INTERVAL_MS) {
          sensor_read_timer_ = 0;
          saveData(sensorData, gps_->read(), gps_->timeString());
        }
        break;
      }

      led_->setColor(0, led_->BYTE_MAX, led_->BYTE_MAX);
      if (sensor_read_timer_ > STATIONARY_READ_INTERVAL_MS) {
        sensor_read_timer_ = 0;
        saveData(sensorData, gps_->read(), gps_->timeString());
      }

    } break;

    case NO_GPS: {
      SensorReading sensorData = readSensors();

      if (gps_->isValid() && !gps_->isOld()) {
        state_ = COLLECTING_DATA;
        led_->setColor(0, led_->BYTE_MAX, 0);
        dataStorage_->logInfo("GPS signal acquired, resuming data collection",
                              gps_->timeString());
      }

      if (checkWifi()) {
        state_ = UPLOADING_DATA;
        break;
      }
    } break;

    case UPLOADING_DATA: {
      if (!dataStorage_->hasData(dataStorage_->DATAFILE)) {
        state_ = IDLE;
        led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, led_->BYTE_MAX);
        break;
      }

      led_->setColor(0, 0, led_->BYTE_MAX);

      std::string wifiMsg =
          "Connected to WiFi: " + std::string(WiFi.SSID().c_str());
      std::string endpointMsg = "Uploading data to " + API_ENDPOINT;
      dataStorage_->logInfo(wifiMsg, gps_->timeString());
      dataStorage_->logInfo(endpointMsg, gps_->timeString());

      if (!registerAndUploadTrip(dataStorage_->DATAFILE)) {
        state_ = ERROR;
        dataStorage_->logError("Failed to upload data, going into error mode");
        break;
      }

      dataStorage_->logInfo("Data upload successful", gps_->timeString());

      if (!dataStorage_->clear(dataStorage_->DATAFILE)) {
        dataStorage_->logError("Failed to clear data storage",
                               gps_->timeString());
        state_ = ERROR;
      }

      dataStorage_->logInfo("Storage cleared", gps_->timeString());

      led_->setColor(led_->BYTE_MAX, led_->BYTE_MAX, led_->BYTE_MAX);
      state_ = IDLE;
    } break;

    case ERROR: {
      led_->setColor(led_->BYTE_MAX, 0, 0);
      dataStorage_->logError("Rebooting device due to error...",
                             gps_->timeString());
      rp2040.reboot();
    } break;
    }

    sleep_ms(1);
  }
}
