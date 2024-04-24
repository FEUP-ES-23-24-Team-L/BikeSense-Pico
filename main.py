#! /usr/bin/env micropython

import random as rand
from bikesense.bikesense import SensorInterface, ReadingResult, GPSModuleInterface, BikeSense

class DummySensorModule(SensorInterface):
   def init(self):
      print("Hello! I'm dummy sensor!")

   def read(self) -> ReadingResult:
      return ReadingResult("DummySensor", rand.randint(0, 100))


class DummyGPSModule(GPSModuleInterface):
   def init(self):
      print("Hello! I'm dummy gps.")

   def read(self):
      return f"gps is at: {rand.random()}"


def main():
   BikeSense(DummyGPSModule()).registerSensor(DummySensorModule()).init().run()


main()