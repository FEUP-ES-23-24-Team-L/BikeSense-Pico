#! /usr/bin/env micropython

import random as rand
from bikesense.bikesense import SensorInterface, ReadingResult, GPSModuleInterface, BikeSense


class DummyGPSModule(GPSModuleInterface):
   def init(self):
      print("Hello! I'm a dummy gps.")

   def read(self):
      return f"gps is at: {rand.random()}"


class DummySensorModule(SensorInterface):
   def init(self):
      print("Hello! I'm a dummy sensor!")

   def read(self) -> ReadingResult:
      return ReadingResult("DummySensor", rand.randint(0, 100))


def main():
   BikeSense(DummyGPSModule()).registerSensor(DummySensorModule()).build().run()


main()