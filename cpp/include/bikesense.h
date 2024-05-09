#ifndef _BIKESENSE_H_
#define _BIKESENSE_H_

#include <elapsedMillis.h>
#include <interfaces.h>
#include <sensorReading.h>

#include <unordered_map>
#include <vector>

#include <HTTPClient.h>
#include <WiFi.h>

typedef std::unordered_map<std::string, std::string> StringMap;

class BikeSense {
private:
  const int SENSOR_READ_INTERVAL_MS;
  const int WIFI_RETRY_INTERVAL_MS;
  const int HTTP_TIMEOUT_MS;
  const int UPLOAD_BATCH_SIZE;

  const std::string &API_TOKEN;
  const std::string &API_ENDPOINT;

  const std::string &BIKE_CODE;
  const std::string &UNIT_CODE;

  bool registered_ = false;
  int bikeId_ = -1;
  int unitId_ = -1;

  std::vector<SensorInterface *> sensors_;
  SensorInterface *gps_;
  DataStorageInterface *dataStorage_;

  HTTPClient http_;
  WiFiMulti multi_;

  void setupSensors();
  SensorReading readSensors() const;

  inline bool checkWifi();

  int registerAndGetID(std::string payload, std::string endpoint);
  int registerTripAndGetID();

  bool uploadAllSensorData();
  int uploadData(const std::vector<SensorReading> &readings);

  elapsedMillis wifi_retry_timer_;

public:
  BikeSense(std::vector<SensorInterface *> sensors, SensorInterface *gps,
            DataStorageInterface *dataStorage, const StringMap &networks,
            const std::string &bikeCode, const std::string &unitCode,
            const std::string &apiAuthToken, const std::string &apiEndpoint,
            const WiFiMode_t wifi_mode = WIFI_STA,
            const int sensor_read_interval_ms = 1000,
            const int wifi_retry_interval_ms = 30000,
            const int http_timeout_ms = 1000, const int upload_batch_size = 10);

  void run();
};

class BikeSenseBuilder {
private:
  std::string bikeCode_;
  std::string unitCode_;

  std::string apiAuthToken_;
  std::string apiEndpoint_;

  std::vector<SensorInterface *> sensors_;
  SensorInterface *gps_;
  DataStorageInterface *dataStorage_;
  StringMap networks_;

public:
  BikeSenseBuilder();
  BikeSenseBuilder &whoAmI(const std::string bikeCode,
                           const std::string unitCode);

  BikeSenseBuilder &addGps(SensorInterface *gps);
  BikeSenseBuilder &addSensor(SensorInterface *sensor);
  BikeSenseBuilder &addDataStorage(DataStorageInterface *dataStorage);

  BikeSenseBuilder &withApiConfig(const std::string &apiToken,
                                  const std::string &apiEndpoint);

  BikeSenseBuilder &addNetwork(const std::string &ssid,
                               const std::string &password);

  BikeSense build();
};

#endif
