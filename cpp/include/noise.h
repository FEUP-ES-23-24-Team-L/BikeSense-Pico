#ifndef _NOISE_H_
#define _NOISE_H_

#include <interfaces.h>

class NoiseSensor : public SensorInterface {
  // TODO: Needs calibration
  const int PIN = 26;
  const int ADC_BIAS = 0;
  const int noiseDBReference = 0;
  const int noiseADCReference = 1;

  const int mvAvgWindowSize = 10;
  int mvAvgAccumulator = 0;

public:
  void setup() override;
  SensorReading read() override;
};

#endif
