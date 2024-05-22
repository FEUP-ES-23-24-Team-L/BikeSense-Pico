#include <Arduino.h>
#include <pico/unique_id.h>

#include "bikesense.h"
#include "gps.h"
#include "infoLed.h"
#include "light.h"
#include "mq2.h"
#include "mq7.h"
#include "noise.h"
#include "sdCard.h"
#include "tempHumidity.h"

#define SERIAL_BAUD 115200

#ifndef UNIT_CODE
#define BIKE_CODE "BSB1"
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

#define API_ENDPOINT "http://192.168.84.175:8080/api/v1"
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

  // digitalWrite(LED_BUILTIN, HIGH);
  // SDCard sd = SDCard();
  // if (!sd.setup()) {
  //   digitalWrite(LED_BUILTIN, LOW);
  //   while (true) {
  //     Serial.println("Failed to setup SD card");
  //     sleep_ms(1000);
  //   }
  // }
  //
  // while (true) {
  //   auto files = sd.getBackUpFiles();
  //   Serial.printf("Files: %d\n", files.size());
  //   for (auto filename : files) {
  //     Serial.printf("Trying to read file: %s\n", filename.c_str());
  //     auto r = sd.retrieve(filename, 5);
  //     if (r.has_value()) {
  //       for (auto l : r.value()) {
  //         Serial.printf("Line: %s\n", l.c_str());
  //       }
  //     } else {
  //       Serial.println("Failed to read file");
  //     }
  //     Serial.println("Done reading file");
  //   }
  //   Serial.println("Done");
  //   sleep_ms(1000);
  // }

  BikeSenseBuilder()
      .addSensor(new NoiseSensor())
      .addSensor(new LightSensor())
      .addSensor(new TempHumiditySensor())
      .addSensor(new MQ2GasSensor())
      .addSensor(new MQ7GasSensor())
      .addDataStorage(new SDCard())
      .addGps(new Gps())
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
