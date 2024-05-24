#ifndef _MQ7_H_
#define _MQ7_H_

#include "elapsedMillis.h"
#include "interfaces.h"

class MQ7GasSensor : public SensorInterface {

private:
  const int MQ7_PIN = A0;
  const int HEAT_CYCLE_TIME = 60 * 1000;
  const int COOL_CYCLE_TIME = 90 * 1000;
  bool isHeatCycle = true;

  elapsedMillis cycleTimer;

public:
  void setup() override;
  SensorReading read() override;
};

#endif
