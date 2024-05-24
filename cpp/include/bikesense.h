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

enum BikeSenseStates {
  IDLE,
  COLLECTING_DATA,
  NO_GPS,
  UPLOADING_DATA,
  ERROR,
};

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

  BikeSenseStates state_ = IDLE;

  bool registered_ = false;
  int bikeId_ = -1;
  int unitId_ = -1;

  std::vector<SensorInterface *> sensors_;
  GpsInterface *gps_;
  DataStorageInterface *dataStorage_;
  LedInterface *led_;

  HTTPClient http_;
  WiFiMulti multi_;

  void setup();
  SensorReading readSensors() const;

  int registerAndGetID(std::string payload, std::string endpoint);
  int registerTripAndGetID();

  bool uploadAllSensorData();
  int uploadData(const std::vector<std::string> &readings);
  int saveData(const SensorReading sensorData, const SensorReading gpsData,
               const std::string timestamp);

public:
  BikeSense(std::vector<SensorInterface *> sensors, GpsInterface *gps,
            DataStorageInterface *dataStorage, LedInterface *led,
            const StringMap &networks, const std::string &bikeCode,
            const std::string &unitCode, const std::string &apiAuthToken,
            const std::string &apiEndpoint,
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
  GpsInterface *gps_;
  DataStorageInterface *dataStorage_;
  LedInterface *led_;
  StringMap networks_;

public:
  BikeSenseBuilder();
  BikeSenseBuilder &whoAmI(const std::string bikeCode,
                           const std::string unitCode);

  BikeSenseBuilder &addGps(GpsInterface *gps);
  BikeSenseBuilder &addSensor(SensorInterface *sensor);
  BikeSenseBuilder &addDataStorage(DataStorageInterface *dataStorage);
  BikeSenseBuilder &addLed(LedInterface *led);

  BikeSenseBuilder &withApiConfig(const std::string &apiToken,
                                  const std::string &apiEndpoint);

  BikeSenseBuilder &addNetwork(const std::string &ssid,
                               const std::string &password);

  BikeSense build();
};

#endif
