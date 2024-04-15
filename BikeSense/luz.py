from machine import Pin, I2C
import lib.si1145 as si1145
import time


i2c = I2C(0, scl=Pin(17), sda=Pin(16), freq=100000)

time.sleep_ms(500)

print(i2c.scan())


sensor = si1145.SI1145(i2c=i2c)


for i in range(10):
    uv = sensor.read_uv
    ir = sensor.read_ir
    view = sensor.read_visible
    print(" UV: %f\n IR: %f\n Visible: %f" % (uv, ir, view))
    time.sleep(1)