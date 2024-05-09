#ifndef _BIKESENSE_H_
#define _BIKESENSE_H_

#include <interfaces.h>
#include <sensorReading.h>

#include <string>
#include <vector>

class BikeSense {
   private:
    std::vector<SensorInterface*> sensors_;
    SensorInterface* gps_;
    DataStorageInterface* dataStorage_;

    const int BIKE_ID;
    const int UNIT_ID;

    const std::string& API_TOKEN;
    const std::string& API_ENDPOINT;

    const std::string& WIFI_SSID;
    const std::string& WIFI_PASSWORD;

    std::vector<SensorReading> readSensors();

    void setupSensors();

   public:
    BikeSense(std::vector<SensorInterface*> sensors, SensorInterface* gps,
              DataStorageInterface* dataStorage, int bikeId, int unitId,
              const std::string& apiToken, const std::string& apiEndpoint,
              const std::string& ssid, const std::string& password)
              ;
    void run();
};

class BikeSenseBuilder {
   private:
    std::vector<SensorInterface*> sensors_;
    SensorInterface* gps_;
    DataStorageInterface* dataStorage_;

    int BIKE_ID;
    int UNIT_ID;

    std::string API_TOKEN;
    std::string API_ENDPOINT;

    std::string WIFI_SSID;
    std::string WIFI_PASSWORD;

   public:
    BikeSenseBuilder();
    BikeSenseBuilder& whoAmI(const int bikeId, const int unitId);

    BikeSenseBuilder& addGps(SensorInterface* gps);
    BikeSenseBuilder& addSensor(SensorInterface* sensor);
    BikeSenseBuilder& addDataStorage(DataStorageInterface* dataStorage);

    BikeSenseBuilder& withApiConfig(const std::string& apiToken,
                                    const std::string& apiEndpoint);

    BikeSenseBuilder& withWifiConfig(const std::string& ssid,
                                     const std::string& password);

    BikeSense build();
};

#endif