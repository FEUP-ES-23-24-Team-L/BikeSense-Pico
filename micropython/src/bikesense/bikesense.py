#! /usr/bin/env micropython

import utime
import machine as m
import network as nw
import urequests as urq
import ujson


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


class DataStorageInterface:
    """
    Interface class that defines the necessary functions for concrete data storage.

    A single data storage module must necessarily be implemented to instantiate the BikeSense Class.
    """

    def init(self):
        pass

    def save(self, data: dict):
        pass

    def read(self) -> list[dict]:
        return list()


class BikeSenseBuilder:
    apiToken: str
    apiEndpoint: str

    bikeID: int
    unitID: int

    wled: m.Pin
    wlan: nw.WLAN
    gps: GPSModuleInterface
    ds: DataStorageInterface
    sensors: list[SensorInterface]

    def __init__(self, gps: GPSModuleInterface, dataStorage: DataStorageInterface):
        self.wlan = nw.WLAN(nw.STA_IF)
        self.wled = m.Pin("LED", m.Pin.OUT)

        self.gps = gps
        self.ds = dataStorage

        self.sensors = list()

    def register_sensor(self, s: SensorInterface):
        self.sensors.append(s)
        return self

    def set_web_api(self, endpoint: str, token: str):
        self.apiEndpoint = endpoint
        self.apiToken = token
        return self

    def set_ids(self, bikeID: int, unitID: int):
        self.bikeID = bikeID
        self.unitID = unitID
        return self

    def connect_wifi(self, ssid: str, password: str):
        self.wlan.active(True)
        self.wlan.connect(ssid, password)
        return self

    def build(self):
        try:
            self.gps.init()
            self.ds.init()

        # TODO: handle this scenario better
        except Exception as e:
            print(f"Exception during init: {e}")
            raise e

        if self.sensors:
            for sensor in self.sensors:
                sensor.init()

        return BikeSense(
            self.wled,
            self.wlan,
            self.gps,
            self.ds,
            self.sensors,
            self.apiToken,
            self.apiEndpoint,
            self.bikeID,
            self.unitID,
        )


class BikeSense:
    """
    BikeSense core logic.
    """

    apiToken: str
    apiEndpoint: str

    bikeID: int
    unitID: int

    tripUploaded: bool

    wled: m.Pin
    wlan: nw.WLAN
    gps: GPSModuleInterface
    ds: DataStorageInterface
    sensors: list[SensorInterface]

    def __init__(
        self,
        wled: m.Pin,
        wlan: nw.WLAN,
        gps: GPSModuleInterface,
        ds: DataStorageInterface,
        sensors: list[SensorInterface],
        apiToken: str,
        apiEndpoint: str,
        bikeID: int,
        unitID: int,
    ):
        self.wled = wled
        self.wlan = wlan
        self.gps = gps
        self.ds = ds
        self.sensors = sensors

        self.apiEndpoint = apiEndpoint
        self.apiToken = apiToken

        self.bikeID = bikeID
        self.unitID = unitID

        self.tripUploaded = False

    # NOTE: Most likely this function will be moved to a separate module / refactored
    #       will probably implement the data upload logic with strategy pattern
    # TODO: Implement handling for the wlan connection status
    def _wifi_is_connected(self) -> bool:
        if self.wlan.isconnected():
            self.wled.on()
            print(f"ip = {self.wlan.ifconfig()[0]}")
            return True
        else:
            self.wled.off()
            print(f"wifi status: {self.wlan.status()}")
            return False

    def _read_sensors(self) -> dict:
        return {r.name: r.value for r in [s.read() for s in self.sensors]}

    def _register_trip(self) -> int:
        json = {"bike_id": self.bikeID, "sensor_unit_id": self.unitID}
        registerTripEndpoint = f"{self.apiEndpoint}/Trip"
        headers = {
            "X-Bike-Sense-Web-Auth": self.apiToken,
            "Content-Type": "application/json",
        }
        response = urq.post(registerTripEndpoint, headers=headers, json=json).json()
        return response["id"]

    def _upload_data(self, tripID: int):
        data = self.ds.read()
        for d in data:
            d.update({"trip_id": tripID})

        response = urq.post(
            f"{self.apiEndpoint}/Trip/UploadData",
            headers={
                "X-Bike-Sense-Web-Auth": self.apiToken,
                "Content-Type": "application/json",
            },
            data=ujson.dumps(data),
        ).text

        self.tripUploaded = True
        print(f"upload response: {response}")

    def _main_loop(self):
        if not self.tripUploaded and self._wifi_is_connected():
            try:
                #NOTE: temporary condition to test the upload logic
                if len(self.ds.read()) > 5:
                    tripID = self._register_trip()
                    self._upload_data(tripID)

            except Exception as e:
                print(f"Exception with get request: {e}")

        gps = self.gps.read()

        readings = self._read_sensors()
        readings.update({gps.name: gps.value})

        time = utime.localtime()
        timeFormated = f"{time[0]:04d}-{time[1]:02d}-{time[2]:02d}T{time[3]:02d}:{time[4]:02d}:{time[5]:02d}Z"
        readings.update({"timestamp": timeFormated})
        self.ds.save(readings)

        print(f"readings: {readings}")

    def run(self, loop_period_ms: int = 1000):
        while True:
            start = utime.ticks_ms()
            self._main_loop()
            delta = utime.ticks_diff(utime.ticks_ms(), start)
            print(f"loop time: {delta}ms\n")
            utime.sleep_ms(max(loop_period_ms - delta, 0))
