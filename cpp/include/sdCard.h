#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <SD.h>
#include <SPI.h>
#include <string>
#include <vector>

#include "interfaces.h"

class SDCard : public DataStorageInterface {
private:
  const int MISO_ = 16;
  const int MOSI_ = 19;
  const int CS_ = 17;
  const int SCK_ = 18;

  const int LOGFILE_MAX_SIZE = 1000000; // 1MB

  bool log(const std::string message);

public:
  bool setup() override;

  retrievedData retrieve(const std::string filename,
                         const int batchSize) override;

  std::vector<std::string> getBackUpFiles() const override;
  bool backupFile(std::string filename, const std::optional<int> trip_id,
                  const std::optional<int> failedBatchIndex) override;

  void decodeFileName(const std::string filename, int &tripId,
                      int &batchIndex) const override;

  bool hasData(std::string filename) const override;

  bool store(const std::string data) override;
  bool clear(std::string filename) override;

  bool logInfo(const std::string message) override;
  bool logInfo(const std::string message, const std::string timestamp) override;

  bool logError(const std::string message) override;
  bool logError(const std::string message,
                const std::string timestamp) override;
};

#endif
