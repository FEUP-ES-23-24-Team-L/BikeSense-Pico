#! /usr/bin/env micropython

import random as rand
from src.bikesense.gps import BksGPS
from src.bikesense.bikesense import (
    SensorInterface,
    ReadingResult,
    BikeSenseBuilder,
    DataStorageInterface,
)


class MockNoiseSensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock noise sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("noise_level", rand.randint(0, 100))


class MockTemperatureSensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock temperature sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("temperature", rand.randint(0, 100))


class MockHumiditySensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock humidity sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("humidity", rand.randint(0, 100))


class MockUVSensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock UV sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("uv_level", rand.randint(0, 100))


class MockLuminositySensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock luminosity sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("luminosity", rand.randint(0, 100))


class MockCarbonSensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock carbon sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("carbon_monoxide_level", rand.randint(0, 100))


class MockPolutionSensor(SensorInterface):
    def init(self):
        print("Hello! I'm a mock polution sensor!")

    def read(self) -> ReadingResult:
        return ReadingResult("polution_particles_ppm", rand.randint(0, 100))


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
    bks.register_sensor(MockNoiseSensor())
    bks.register_sensor(MockTemperatureSensor())
    bks.register_sensor(MockHumiditySensor())
    bks.register_sensor(MockUVSensor())
    bks.register_sensor(MockLuminositySensor())
    bks.register_sensor(MockCarbonSensor())
    bks.register_sensor(MockPolutionSensor())
    bks.connect_wifi("ArchBtw", "123arch321").build().run()


main()
