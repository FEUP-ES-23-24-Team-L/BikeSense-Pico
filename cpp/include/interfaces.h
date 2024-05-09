#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <sensorReading.h>

#include <vector>

class SensorInterface {
public:
  virtual void setup() = 0;
  virtual SensorReading read() const = 0;
};

typedef std::optional<std::vector<SensorReading>> retrievedData;

class DataStorageInterface {
public:
  virtual void setup() = 0;
  virtual void store(const SensorReading &reading) = 0;
  virtual retrievedData retrieve(int batchSize) = 0;
};

#endif
