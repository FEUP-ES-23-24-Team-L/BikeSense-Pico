#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <sensorReading.h>

#include <Arduino.h>
#include <optional>
#include <vector>

class SensorInterface {
public:
  virtual void setup() = 0;
  virtual SensorReading read() = 0;
};

class GpsInterface : public SensorInterface {
public:
  virtual bool isOld() = 0;
  virtual void update() = 0;
  virtual bool isValid() = 0;
  virtual bool isMoving() = 0;
  virtual bool isUpdated() = 0;

  virtual std::string timeString() = 0;
};

typedef std::optional<std::vector<std::string>> retrievedData;

class DataStorageInterface {
public:
  std::string DATAFILE = "Bikesense.txt";
  std::string LOGFILE = "Bikesense_Logs.txt";

  virtual bool setup() = 0;

  virtual retrievedData retrieve(std::string filename, const int batchSize) = 0;

  virtual std::vector<std::string> getBackUpFiles() const = 0;
  virtual bool backupFile(std::string filename, const std::optional<int> tripId,
                          const std::optional<int> failedBatchIndex) = 0;

  virtual void decodeFileName(const std::string filename, int &tripId,
                              int &batchIndex) const = 0;

  virtual bool hasData(std::string filename) const = 0;

  virtual bool store(const std::string data) = 0;
  virtual bool clear(std::string filename) = 0;

  virtual bool logInfo(const std::string message) = 0;
  virtual bool logInfo(const std::string message,
                       const std::string timestamp) = 0;

  virtual bool logError(const std::string message) = 0;
  virtual bool logError(const std::string message,
                        const std::string timestamp) = 0;
};

class LedInterface {
public:
  const byte BYTE_MAX = 255;

  virtual void setup() = 0;
  virtual void setColor(byte r, byte g, byte b) = 0;
};

#endif
