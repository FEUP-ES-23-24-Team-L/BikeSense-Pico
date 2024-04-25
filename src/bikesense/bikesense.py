#! /usr/bin/env micropython

import utime
import machine as m
import network as nw
import urequests as urq


class ReadingResult:
    """
    Return type of SensorInterface's read() function.
    """

    name: str
    value: object

    def __init__(self, name: str, value: object):
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
        return ReadingResult("", "")


class GPSModuleInterface:
    """
    Interface class that defines the necessary functions for concrete sensors.

    A single GPS module must necessarily be implemented to instantiate the BikeSense Class.
    """

    def init(self):
        pass

    def read(self) -> ReadingResult:
        return ReadingResult("", "")



class BikeSenseBuilder:
    wled: m.Pin
    wlan: nw.WLAN
    gps: GPSModuleInterface
    sensors: list[SensorInterface]

    def __init__(self, gps: GPSModuleInterface):
        self.wlan = nw.WLAN(nw.STA_IF)
        self.wled = m.Pin("LED", m.Pin.OUT)
        self.gps = gps
        self.sensors = list()

    def registerSensor(self, s: SensorInterface):
        self.sensors.append(s)
        return self

    def connectWifi(self, ssid: str, password: str):
        self.wlan.active(True)
        self.wlan.connect(ssid, password)
        return self

    def build(self):
        self.gps.init()

        if self.sensors:
            for sensor in self.sensors:
                sensor.init()

        return BikeSense(self.wled, self.wlan, self.gps, self.sensors)


class BikeSense:
    """
    BikeSense core logic.
    """

    wled: m.Pin
    wlan: nw.WLAN
    gps: GPSModuleInterface
    sensors: list[SensorInterface]

    def __init__(
        self,
        wled: m.Pin,
        wlan: nw.WLAN,
        gps: GPSModuleInterface,
        sensors: list[SensorInterface],
    ):
        self.wled = wled
        self.wlan = wlan
        self.gps = gps
        self.sensors = sensors

    def _wifiIsConnected(self) -> bool:
        if self.wlan.isconnected():
            self.wled.on()
            print(f"ip = {self.wlan.ifconfig()[0]}")
            return True
        else:
            self.wled.off()
            print(f"wifi status: {self.wlan.status()}")
            return False

    def run(self, loop_delay_ms: int = 1000):
        while True:
            #TODO: change sleep to timer based approach
            #      Make http related stuff async
            utime.sleep_ms(loop_delay_ms)

            if self._wifiIsConnected():
                try:
                    r = urq.get("http://date.jsontest.com")
                    print(f"get response: {r.json()}")
                except Exception as e:
                    print(f"Exception with get request: {e}")

            gps = self.gps.read()
            print(f"{gps}")

            for s in self.sensors:
                reading = s.read()
                print(f"{reading.name}: {reading.value}\n")
