from machine import UART, Pin

from src.bikesense.bikesense import GPSModuleInterface
from src.thirdParty.micropyGPS import MicropyGPS


class BksGPS(GPSModuleInterface):
    microGps: MicropyGPS
    uart0: UART

    def init(self):
        self.uart0 = UART(0, baudrate=9600, tx=Pin(16), rx=Pin(17), timeout=2)
        self.microGps = MicropyGPS()

    def read(self) -> str:

        if self.uart0.any() > 0:
            rxData = str(self.uart0.readline())[2:][:-5]
            # print(rxData)
            for x in rxData:
                self.microGps.update(x)

        latitude = self.microGps.latitude
        longitude = self.microGps.longitude
        altitude = self.microGps.altitude

        return f"lat:{latitude} | long:{longitude} | alt:{altitude}"
