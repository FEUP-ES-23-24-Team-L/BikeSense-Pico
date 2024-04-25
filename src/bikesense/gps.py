from machine import UART, Pin

from src.bikesense.bikesense import GPSModuleInterface, ReadingResult
from src.thirdParty.micropyGPS import MicropyGPS


class BksGPS(GPSModuleInterface):
    microGps: MicropyGPS
    uart0: UART

    def init(self):
        self.uart0 = UART(0, baudrate=9600, tx=Pin(16), rx=Pin(17), timeout=2)
        self.microGps = MicropyGPS()

    def read(self) -> ReadingResult:

        if self.uart0.any() > 0:
            rxData = str(self.uart0.readline())[2:][:-5]
            for x in rxData:
                self.microGps.update(x)

        #TODO: refactor this according to BikeSense-Web api
        latitude = self.microGps.latitude
        longitude = self.microGps.longitude
        altitude = self.microGps.altitude
        val = f"lat:{latitude} | long:{longitude} | alt:{altitude}"
        return  ReadingResult("GPS", val)
