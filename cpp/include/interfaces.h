#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <sensorReading.h>

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
  virtual std::string timeString() = 0;
};

typedef std::optional<std::vector<std::string>> retrievedData;

class DataStorageInterface {
public:
  virtual bool setup() = 0;
  virtual void store(const std::string data) = 0;
  virtual void clear() = 0;
  virtual retrievedData retrieve(int batchSize) = 0;
};

#endif
