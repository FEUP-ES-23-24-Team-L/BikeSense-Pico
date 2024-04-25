#! /usr/bin/env micropython

import random as rand
from src.bikesense.bikesense import (
    SensorInterface,
    ReadingResult,
    BikeSense,
)
from src.bikesense.gps import BksGPS


class DummySensorModule(SensorInterface):
    def init(self):
        print("Hello! I'm a dummy sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("DummySensor", rand.randint(0, 100))


def main():
    BikeSense(BksGPS()).registerSensor(DummySensorModule()).build().run()


main()
