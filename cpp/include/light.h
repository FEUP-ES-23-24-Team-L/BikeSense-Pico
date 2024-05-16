#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "SI114X.h"
#include "interfaces.h"

class LightSensor : public SensorInterface {
  const int SDA_PIN = 4;
  const int SCL_PIN = 5;

  SI114X SI1145 = SI114X();
  bool initialized = false;

  void setup();
  SensorReading read();
};

#endif // !_LIGHT_H_
