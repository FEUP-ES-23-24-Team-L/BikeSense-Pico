#include <Arduino.h>
#include <bikesense.h>
#include <mock.h>

#define SERIAL_BAUD 9600

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println("BikeSense is starting...");
}

void loop() {
    BikeSenseBuilder()
        .addSensor(new MockSensor())
        .addGps(new MockGps())
        .addDataStorage(new MockDataStorage())
        .whoAmI(1, 1)
        .withApiConfig("api_token", "api_endpoint")
        .withWifiConfig("wifi_ssid", "wifi_password")
        .build()
        .run();
}