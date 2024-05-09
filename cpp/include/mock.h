#ifndef _MOCK_H_
#define _MOCK_H_

#include <interfaces.h>

#include <vector>

class MockSensor : public SensorInterface {
public:
  void setup() override;
  SensorReading read() const override;
};

class MockGps : public SensorInterface {
public:
  void setup() override;
  SensorReading read() const override;
};

class MockDataStorage : public DataStorageInterface {
private:
  std::vector<SensorReading> readings_;

public:
  void setup() override;
  void store(const SensorReading &reading) override;
  retrievedData retrieve(int batchSize) override;
};

#endif
