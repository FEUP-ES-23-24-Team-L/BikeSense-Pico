from machine import Pin, I2C, RTC
from lib.ds3231 import DS3231

#first use REPL in the ide to sync RTC with local machine

i2c = I2C(0, scl=Pin(21), sda=Pin(20), freq=100000)

ds = ds = DS3231(i2c)

rtc = RTC()

print("interno")
print(rtc.datetime())

(year, month, day, wday, hour, minute, second, microsecond) = rtc.datetime()

ds.datetime((year, month, day, hour, minute, second, wday))

print("externo")
print("year, month, day, weekday, hour, minute, second, 0")
print(ds.datetime())

