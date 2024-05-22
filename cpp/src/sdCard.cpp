#include "sdCard.h"
#include "interfaces.h"
#include <SD.h>
#include <SPI.h>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>

bool SDCard::setup() {
  SPI.setRX(MISO_);
  SPI.setTX(MOSI_);
  SPI.setSCK(SCK_);

  if (!SD.begin(CS_)) {
    Serial.println("SD Card initialization failed!");
    return false;
  }

  File logFile = SD.open(LOGFILE.c_str(), FILE_WRITE);
  if (!logFile) {
    Serial.println("Failed to open log file!");
    return false;
  }

  if (logFile.size() > LOGFILE_MAX_SIZE) {
    Serial.println("Log file size exceeded, clearing log file");
    logFile.close();

    if (!clear(LOGFILE)) {
      Serial.println("Failed to clear log file");
      return false;
    }
  } else
    logFile.close();

  if (SD.exists(DATAFILE.c_str())) {
    File file = SD.open(DATAFILE.c_str(), FILE_READ);
    if (!file) {
      logError("Failed to open data file");
      return false;
    }

    if (file.size() > 0) {
      logInfo("Data file exists, backing up to prepare for new trip");
      file.close();

      if (!backupFile(DATAFILE, std::nullopt, std::nullopt)) {
        return false;
      }

      SD.remove(DATAFILE.c_str());
    } else {
      file.close();
    }
  }

  return true;
}

bool SDCard::hasData(std::string filename) const {
  if (!SD.exists(filename.c_str())) {
    return false;
  }

  File file = SD.open(filename.c_str(), FILE_READ);
  if (!file) {
    return false;
  }

  size_t size = file.size();
  file.close();
  return size > 0;
}

bool SDCard::store(const std::string data) {
  File f = SD.open(DATAFILE.c_str(), FILE_WRITE);
  if (!f) {
    Serial.println("Error opening file for writing");
    return false;
  }

  f.println(data.c_str());
  f.close();
  return true;
}

bool SDCard::backupFile(std::string filename, const std::optional<int> tripID,
                        const std::optional<int> failedBatchIndex) {
  File f = SD.open(filename.c_str(), FILE_READ);
  if (!f)
    return false;
  f.close();

  if (!SD.exists("backups")) {
    if (!SD.mkdir("backups")) {
      logError("Error creating backups directory");
      return false;
    }
  }

  std::string backupDIR = "backups";
  std::string backupFile = "/bikesense" + std::to_string(millis());
  if (tripID.has_value()) {
    backupFile += "-" + std::to_string(tripID.value());
  }
  if (failedBatchIndex.has_value()) {
    backupFile += "-" + std::to_string(failedBatchIndex.value());
  }
  backupFile += ".txt";

  logInfo("Backing up trip data to " + backupDIR + backupFile);
  if (!SD.rename(filename.c_str(), (backupDIR + backupFile).c_str())) {
    logError("Error renaming file trip data");
    return false;
  }

  return true;
}

void SDCard::decodeFileName(const std::string filename, int &tripId,
                            int &batchIndex) const {
  // Split filename by '-'
  std::istringstream ss(filename);
  char delimiter = '-';

  std::vector<std::string> parts;
  std::string part;

  while (std::getline(ss, part, delimiter)) {
    parts.push_back(part);
  }

  if (parts.size() == 2) {
    tripId = std::stoi(parts[1]);
    batchIndex = -1;
  } else if (parts.size() == 3) {
    tripId = std::stoi(parts[1]);
    batchIndex = std::stoi(parts[2]);
  } else {
    tripId = -1;
    batchIndex = -1;
  }
}

std::vector<std::string> SDCard::getBackUpFiles() const {
  std::vector<std::string> backupFiles;
  std::string dirname = "backups";
  File dir = SD.open(dirname.c_str());
  while (true) {
    File file = dir.openNextFile();
    if (!file) {
      break;
    }

    if (file.isDirectory() || file.size() == 0) {
      file.close();
      continue;
    }

    std::string fullPath = dirname + "/" + file.name();
    backupFiles.push_back(fullPath.c_str());
    file.close();
  }

  dir.close();
  return backupFiles;
}

retrievedData SDCard::retrieve(std::string filename_, int batchSize) {
  static std::map<std::string, int> lastReadPositions;
  std::string filename = std::string(filename_);
  if (lastReadPositions.find(filename) == lastReadPositions.end()) {
    Serial.printf("Initializing last read position for file %s\n",
                  filename.c_str());
    lastReadPositions[filename] = 0;
  }
  int lastReadPosition = lastReadPositions[filename];
  Serial.printf("Last read position for file %s is %d\n", filename.c_str(),
                lastReadPosition);

  File f = SD.open(filename.c_str(), FILE_READ);
  if (!f) {
    return std::nullopt;
  }

  if (f.position() >= f.size()) {
    lastReadPosition = 0;
    this->logInfo("End of file reached, resetting read position");
    f.close();
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
    f.close();
    return std::nullopt;
  }

  Serial.printf("Last read position for file %s is now %d\n", filename.c_str(),
                f.position());
  lastReadPositions[filename] = f.position();
  f.close();
  return data;
}

bool SDCard::clear(std::string filename) {
  if (SD.exists(filename.c_str())) {
    if (SD.remove(filename.c_str())) {
      logInfo("File " + std::string(filename) + " cleared successfully");
      return true;
    }
    logError("Error clearing file " + std::string(filename));
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
  if (timestamp.empty()) {
    return logInfo(message);
  }
  std::string infoMsg = "[" + timestamp + "] " + message;
  return logInfo(infoMsg);
}

bool SDCard::logError(const std::string message) {
  std::string errorMsg =
      "[" + std::to_string(millis()) + "] " + "[ERROR] " + message;
  Serial.println(errorMsg.c_str());

  return log(errorMsg);
}

bool SDCard::logError(const std::string message, const std::string timestamp) {
  if (timestamp.empty()) {
    return logError(message);
  }
  std::string errorMsg = "[" + timestamp + "] " + message;
  return logError(errorMsg);
}

bool SDCard::log(const std::string message) {
  Serial.println(message.c_str());

  File f = SD.open(LOGFILE.c_str(), FILE_WRITE);
  if (!f) {
    return false;
  }
  f.println(message.c_str());
  f.close();
  return true;
}
