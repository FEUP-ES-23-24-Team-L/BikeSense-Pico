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

  // TODO: is this right?
  File logFile = SD.open(DATAFILE, FILE_WRITE);
  if (!logFile) {
    Serial.println("Error opening log file!");
    return false;
  }

  if (logFile.size() > LOGFILE_MAX_SIZE && !logFile.truncate(0)) {
    Serial.println("Error truncating log file!");
    logFile.close();
    return false;
  }

  logFile.close();

  return true;
}

bool SDCard::store(const std::string data) {
  File f = SD.open(DATAFILE, FILE_WRITE);
  if (f) {
    f.println(data.c_str());
    Serial.println("Data stored successfully");
  } else {
    Serial.println("Error opening file for writing");
    return false;
  }
  f.close();
  return true;
}

bool SDCard::backupTripData(const std::optional<int> trip_id) {
  File f = SD.open(DATAFILE, FILE_READ);
  if (!f)
    return false;

  std::string backupFile = "BikesenseBackup" + std::to_string(millis());
  if (trip_id.has_value()) {
    backupFile += "-" + std::to_string(trip_id.value());
  }
  backupFile += ".txt";
  std::string backupDIR = "/backup/" + backupFile;

  return SD.rename(DATAFILE, (backupDIR + backupFile).c_str());
}

std::vector<std::string> SDCard::getBackUpFiles() const {
  std::vector<std::string> backupFiles;
  File root = SD.open("/backup/");
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      file = root.openNextFile();
      continue;
    }

    backupFiles.push_back(file.name());
    file = root.openNextFile();
  }
  return backupFiles;
}

retrievedData SDCard::retrieve(int batchSize) {
  return retrieve(batchSize, DATAFILE);
}

retrievedData SDCard::retrieve(int batchSize, const char *filename) {
  static std::map<const char *, int> lastReadPositions;
  if (lastReadPositions.find(filename) == lastReadPositions.end()) {
    lastReadPositions[filename] = 0;
  }
  int lastReadPosition = lastReadPositions[filename];

  File f = SD.open(filename, FILE_READ);
  if (!f) {
    return std::nullopt;
  }

  if (f.position() >= f.size()) {
    lastReadPosition = 0;
    this->logInfo("End of file reached, resetting read position");
    return std::nullopt;
  }

  if (f.size() == 0) {
    f.close();
    lastReadPosition = 0;
    return std::nullopt;
  }

  f.seek(lastReadPosition);
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

  lastReadPositions[filename] = f.position();
  f.close();
  return data;
}

bool SDCard::clear() {
  if (SD.exists(DATAFILE)) {
    SD.remove(DATAFILE);
    return true;
  }

  return false;
}

bool SDCard::logInfo(const std::string message) {
  std::string infoMsg =
      "[" + std::to_string(millis()) + "] " + "[INFO] " + message;
  Serial.println(infoMsg.c_str());

  return log(infoMsg);
}

bool SDCard::logInfo(const std::string message, const std::string timestamp) {
  std::string infoMsg = "[" + std::to_string(millis()) + "] [" + timestamp +
                        "] [INFO] " + message;
  return logInfo(infoMsg);
}

bool SDCard::logError(const std::string message) {
  std::string errorMsg =
      "[" + std::to_string(millis()) + "] " + "[ERROR] " + message;
  Serial.println(errorMsg.c_str());

  return log(errorMsg);
}

bool SDCard::logError(const std::string message, const std::string timestamp) {
  std::string errorMsg = "[" + std::to_string(millis()) + "] [" + timestamp +
                         "] [ERROR] " + message;
  return logError(errorMsg);
}

bool SDCard::log(const std::string message) {
  Serial.println(message.c_str());

  File f = SD.open(LOGFILE, FILE_WRITE);
  if (!f) {
    return false;
  }
  f.println(message.c_str());
  f.close();
  return true;
}
