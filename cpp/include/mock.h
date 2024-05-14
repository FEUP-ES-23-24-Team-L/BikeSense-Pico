#ifndef _MOCK_H_
#define _MOCK_H_

#include <interfaces.h>

#include <vector>

class MockSensor : public SensorInterface {
public:
  void setup() override;
  SensorReading read() override;
};

class MockGps : public GpsInterface {
public:
  void setup() override;
  SensorReading read() override;
  std::string timeString() override;
};

class MockDataStorage : public DataStorageInterface {
private:
  std::vector<std::string> readings_;

public:
  bool setup() override;
  void store(const std::string reading) override;
  retrievedData retrieve(int batchSize) override;
};

#endif
