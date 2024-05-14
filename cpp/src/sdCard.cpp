#include "sdCard.h"
#include <SD.h>
#include <SPI.h>

bool SDCard::setup() {
  SPI.setRX(MISO_);
  SPI.setTX(MOSI_);
  SPI.setSCK(SCK_);

  if (!SD.begin(CS_)) {
    Serial.println("SD Card initialization failed!");
    return false;
  }

  Serial.println("SD Card initialized!");
  return true;
}

void SDCard::store(const std::string data) {
  File f = SD.open("Bikesense.txt", FILE_WRITE);
  if (f) {
    f.println(data.c_str());
  } else {
    Serial.println("Failed to open file for writing");
  }
  f.close();
}

retrievedData SDCard::retrieve(int batchSize) {
  File f = SD.open("Bikesense.txt", FILE_READ);
  if (!f) {
    Serial.println("Failed to open file for reading");
    return std::nullopt;
  }

  if (f.size() == 0) {
    f.close();
    lastReadPosition_ = 0;
    return std::nullopt;
  }

  f.seek(lastReadPosition_);
  std::vector<std::string> data;
  for (int i = 0; i < batchSize; i++) {
    if (f.available()) {
      data.push_back(f.readStringUntil('\n').c_str());
    } else {
      break;
    }
  }

  if (data.empty()) {
    return std::nullopt;
  }

  lastReadPosition_ = f.position();

  if (f.position() >= f.size()) {
    lastReadPosition_ = 0;
    Serial.println("Reached end of file, resetting read position...");
  }
  f.close();

  return data;
}

void SDCard::clear() {
  File f = SD.open("Bikesense.txt", FILE_WRITE);
  f.truncate(0);
  f.close();
  Serial.println("Data file cleared!");
}
