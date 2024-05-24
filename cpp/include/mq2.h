#ifndef _MQ2_H_
#define _MQ2_H_

#include "elapsedMillis.h"
#include "interfaces.h"

class MQ2GasSensor : public SensorInterface {

private:
  const int MQ2_PIN = A1;
  const int HEAT_CYCLE_TIME = 60 * 1000;
  const int COOL_CYCLE_TIME = 90 * 1000;
  bool isHeatCycle = true;

  elapsedMillis cycleTimer;

public:
  void setup() override;
  SensorReading read() override;
};

#endif // _MQ2_H_
