#include "infoLed.h"
#include "light.h"
#include "pico/unique_id.h"
#include "tempHumidity.h"
#include <Arduino.h>
#include <bikesense.h>
#include <gps.h>
#include <mock.h>
#include <noise.h>
#include <sdCard.h>

#define SERIAL_BAUD 115200

#ifndef UNIT_CODE
#define BIKE_CODE "BSB1"
#endif

#ifndef STASSID_DEFAULT
#define STASSID_DEFAULT "bikenet"
#define STAPSK_DEFAULT "Bike123!"
#endif

// #define LOCAL_TEST_MODE

#ifndef LOCAL_TEST_MODE
#define API_ENDPOINT "http://10.227.103.175:8080/api/v1"
#define API_TOKEN "NotARealToken"
#endif

#ifdef LOCAL_TEST_MODE
#define STASSID_TEST "ArchBtw"
#define STAPSK_TEST "123arch321"

#define API_ENDPOINT "http://192.168.84.176:8080/api/v1"
#define API_TOKEN "TestToken"
#endif

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("BikeSense is starting...");
}

void loop() {
  uint ID_LEN = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1;
  char id[ID_LEN];
  pico_get_unique_board_id_string(id, ID_LEN);
  digitalWrite(LED_BUILTIN, HIGH);

  BikeSenseBuilder()
      .addSensor(new MockSensor())
      .addSensor(new NoiseSensor())
      .addSensor(new LightSensor())
      .addSensor(new TempHumiditySensor())
      .addGps(new Gps())
      .addDataStorage(new SDCard())
      .addLed(new InfoLed())
      .whoAmI(BIKE_CODE, id)
      .withApiConfig(API_TOKEN, API_ENDPOINT)
      .addNetwork(STASSID_DEFAULT, STAPSK_DEFAULT)
#ifdef LOCAL_TEST_MODE
      .addNetwork(STASSID_TEST, STAPSK_TEST)
#endif
      .build()
      .run();
}
