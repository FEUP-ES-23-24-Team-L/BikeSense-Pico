#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <sensorReading.h>

#include <Arduino.h>
#include <vector>

class SensorInterface {
public:
  virtual void setup() = 0;
  virtual SensorReading read() = 0;
};

class GpsInterface : public SensorInterface {
public:
  virtual void update() = 0;
  virtual bool isValid() = 0;
  virtual bool isUpdated() = 0;
  virtual std::string timeString() = 0;
};

typedef std::optional<std::vector<std::string>> retrievedData;

class DataStorageInterface {
public:
  virtual bool setup() = 0;

  virtual retrievedData retrieve(int batchSize) = 0;
  virtual bool store(const std::string data) = 0;
  virtual bool clear() = 0;

  virtual bool logInfo(const std::string message) = 0;
  virtual bool logError(const std::string message) = 0;
  virtual bool logDumpOverSerial() = 0;
};

class LedInterface {
public:
  const byte BYTE_MAX = 255;

  virtual void setup() = 0;
  virtual void setColor(byte r, byte g, byte b) = 0;
};

#endif
