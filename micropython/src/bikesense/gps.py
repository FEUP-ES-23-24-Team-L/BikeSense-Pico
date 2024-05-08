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

        # TODO: refactor this according to BikeSense-Web api

        data = {
            "latitude": convert_from_tuple(self.microGps.latitude),
            "longitude": convert_from_tuple(self.microGps.longitude),
            "altitude": self.microGps.altitude,
            "speed": self.microGps.speed[2], # speed over ground (km/h)
            "course": self.microGps.course,  # course over ground (degrees)
            "satellites_in_use": self.microGps.satellites_in_use,
            "fix_type": self.microGps.fix_type,  # 0: No fix, 1: 2D fix, 2: 3D fix
            "hdop": self.microGps.hdop,  # horizontal dilution of precision (HDOP)
            "vdop": self.microGps.vdop,  # vertical dilution of precision (VDOP)
            "pdop": self.microGps.pdop,  # position dilution of precision (PDOP)
        }
        # Note: for DOP values, the lower the value, the better the accuracy
        # Values close to 1 are ideal

        return ReadingResult("gps_data", data)


def convert_from_tuple(coord_tuple) -> float:
    """
    Converts latitude/longitude in degrees-minutes-direction tuple format
    to a single numeric value.

    Args:
        coord_tuple: A tuple representing the coordinate (degrees, minutes, direction).
                Example: (37, 51.65, 'S')

    Returns:
        A float representing the decimal value of the coordinate.
    """
    degrees, minutes, direction = coord_tuple
    multiplier = 1 if direction in ["N", "E"] else -1
    return float(degrees) + minutes / 60 * multiplier
