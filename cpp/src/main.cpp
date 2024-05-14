#include <Arduino.h>
#include <bikesense.h>
#include <gps.h>
#include <mock.h>
#include <noise.h>
#include <sdCard.h>

#define SERIAL_BAUD 9600

#ifndef UNIT_CODE
#define BIKE_CODE "BSB1"
#define UNIT_CODE "BSU1"
#endif

#ifndef STASSID_DEFAULT
#define STASSID_DEFAULT "bikenet"
#define STAPSK_DEFAULT "Bike123!"
#endif

#define LOCAL_TEST_MODE

#ifndef LOCAL_TEST_MODE
#define API_ENDPOINT "http://10.227.103.175:8080/api/v1"
#define API_TOKEN "NotARealToken"
#endif

#ifdef LOCAL_TEST_MODE
#define STASSID_TEST "ArchBtw"
#define STAPSK_TEST "123arch321"

#define API_ENDPOINT "http://192.168.163.175:8080/api/v1"
#define API_TOKEN "TestToken"
#endif

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("BikeSense is starting...");
}

void loop() {
  BikeSenseBuilder()
      .addSensor(new MockSensor())
      .addSensor(new NoiseSensor())
      .addGps(new Gps())
      .addDataStorage(new SDCard())
      .whoAmI(BIKE_CODE, UNIT_CODE)
      .withApiConfig(API_TOKEN, API_ENDPOINT)
      .addNetwork(STASSID_DEFAULT, STAPSK_DEFAULT)
#ifdef LOCAL_TEST_MODE
      .addNetwork(STASSID_TEST, STAPSK_TEST)
#endif
      .build()
      .run();
}
