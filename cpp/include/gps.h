#ifndef _GPS_H_
#define _GPS_H_

#include <TinyGPS++.h>
#include <interfaces.h>
#include <sensorReading.h>

class Gps : public GpsInterface {
private:
  TinyGPSPlus gps_;

public:
  void setup() override;
  void update() override;
  bool isValid() override;
  SensorReading read() override;
  std::string timeString() override;
};

#endif // !_GPS_H_
