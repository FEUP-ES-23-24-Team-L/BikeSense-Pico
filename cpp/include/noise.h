#ifndef _NOISE_H_
#define _NOISE_H_

#include <interfaces.h>

class NoiseSensor : public SensorInterface {
public:
  void setup() override;
  SensorReading read() override;
};

#endif
