from machine import Pin, I2C
from lib.ds3231 import DS3231


i2c = I2C(0, scl=Pin(21), sda=Pin(20), freq=100000)

ds = ds = DS3231(i2c)

(year, month, day, wday, hour, minute, second, _) = ds.datetime() #type: ignore

print((year, month, day, wday, hour, minute, second))