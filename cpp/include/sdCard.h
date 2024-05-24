#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <SD.h>
#include <SPI.h>

#include "interfaces.h"

class SDCard : public DataStorageInterface {
private:
  const int MISO_ = 16;
  const int MOSI_ = 19;
  const int CS_ = 17;
  const int SCK_ = 18;

  const char *DATAFILE = "Bikesense.txt";
  const char *LOGFILE = "Bikesense_Logs.txt";

  const int LOGFILE_MAX_SIZE = 1000000; // 1MB

  size_t lastReadPosition_ = 0;

  bool log(const std::string message);

public:
  bool setup() override;

  retrievedData retrieve(int batchSize) override;
  bool store(const std::string data) override;
  bool clear() override;

  bool logInfo(const std::string message) override;
  bool logInfo(const std::string message, const std::string timestamp) override;

  bool logError(const std::string message) override;
  bool logError(const std::string message,
                const std::string timestamp) override;
};

#endif
