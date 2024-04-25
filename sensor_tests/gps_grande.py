from machine import UART, Pin

from src.thirdParty.micropyGPS import MicropyGPS

import time

uart0 = UART(0, baudrate=9600, tx=Pin(16), rx=Pin(17), timeout=2)

my_gps = MicropyGPS()

now = time.ticks_ms()
last = now

while True:
    if(uart0.any() > 0):
        rxData = str(uart0.readline())[2:][:-5]
        print(rxData)
        for x in rxData:
            my_gps.update(x)

    if(time.ticks_ms() - last > 1000):
        print(my_gps.longitude)
        print(my_gps.latitude)
        last = time.ticks_ms()