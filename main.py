#! /usr/bin/env micropython

import random as rand
from src.bikesense.gps import BksGPS
from src.bikesense.bikesense import (
    SensorInterface,
    ReadingResult,
    BikeSenseBuilder,
    DataStorageInterface,
)


class DummySensorModule(SensorInterface):
    def init(self):
        print("Hello! I'm a dummy sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("DummySensor", rand.randint(0, 100))


class MockDataStorage(DataStorageInterface):
    data: list[dict]

    def init(self):
        self.data = list()

    def save(self, data: dict):
        self.data.append(data)

        # NOTE:  Since this is a mock storage, we will only keep the last 10 readings
        #        in order to avoid memory issues. Final implementation should save
        #        data to an SD card.
        #
        # TODO:  Look into SD card interfacing with micropython.
        #        Look into MicroFat file system.
        if len(self.data) > 10:
            self.data.pop(0)

    # NOTE: Instead of returning the entire database, the final implementation
    #       should return the data in chunks (e.g. 10 readings at a time)
    #       in order to avoid memory issues and facilitate data upload.
    #
    # TODO: Maybe adapt the data storage interface to return data in chunks.
    def read(self) -> list[dict]:
        return self.data


def main():
    bks = BikeSenseBuilder(BksGPS(), MockDataStorage())
    bks.register_sensor(DummySensorModule())
    bks.connect_wifi("ArchBtw", "123arch321").build().run()


main()
