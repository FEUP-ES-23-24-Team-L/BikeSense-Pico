#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <sensorReading.h>

#include <vector>

class SensorInterface {
   public:
    virtual void setup() = 0;
    virtual SensorReading read() = 0;
};

class DataStorageInterface {
   public:
    virtual void setup() = 0;
    virtual void store(const SensorReading& reading) = 0;
    virtual std::vector<SensorReading> retrieve(int batchSize) = 0;
};

#endif