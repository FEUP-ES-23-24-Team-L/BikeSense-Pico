#ifndef _GPS_H_
#define _GPS_H_

#include <TinyGPS++.h>
#include <interfaces.h>
#include <sensorReading.h>

class Gps : public GpsInterface {
private:
  const int MAX_READING_AGE_MS = 5000;

  TinyGPSPlus gps_;
  char buffer_[1000];
  char bufferIndex_ = 0;

public:
  void setup() override;
  void update() override;
  bool isValid() override;
  bool isUpdated() override;
  bool isOld() override;
  SensorReading read() override;
  std::string timeString() override;
};

#endif // !_GPS_H_
