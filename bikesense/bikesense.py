#! /usr/bin/env micropython

import time


class ReadingResult():
   """
   Return type of SensorInterface's read() function.
   """

   name: str
   value: any

   def __init__(self, name: str, value: any):
      """
      Initialize a ReadingResult instance.

      Args:
          name (str): measurement / sensor name
          value (any): measurement value
      """
      self.name = name
      self.value = value


class SensorInterface:
   """
   Interface class that defines the necessary functions for concrete sensors.

   Instantiate as many of these as needed for the concrete sensor types.
   """

   def init(self):
      pass

   def read(self) -> ReadingResult:
      pass


class GPSModuleInterface:
   """
   Interface class that defines the necessary functions for concrete sensors.
   
   A single GPS module must necessarily be implemented to instantiate the BikeSense Class.
   """

   def init(self):
      pass

   def read(self) -> str:
      pass


class BikeSense:
   """
   BikeSense core logic.
   """

   gps: GPSModuleInterface
   sensors: list[SensorInterface] | None

   def __init__(self, gps: GPSModuleInterface):
      self.gps = gps
      self.sensors = None

   def registerSensor(self, s: SensorInterface):
      if self.sensors is None:
         self.sensors = list()
      
      self.sensors.append(s)
      return self

   def build(self):
      self.gps.init()

      if self.sensors:
         for sensor in self.sensors:
            sensor.init()

      return self
   
   def run(self, loop_delay_ms: int = 1000):
      while True:
         time.sleep(loop_delay_ms / 1000)

         gps = self.gps.read()
         print(f"{gps}")

         if self.sensors is None:
            continue

         for s in self.sensors:
            reading = s.read()
            print(f"{reading.name}: {reading.value}\n")
