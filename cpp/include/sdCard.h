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

  size_t lastReadPosition_ = 0;

public:
  bool setup() override;
  void store(const std::string data) override;
  void clear() override;
  retrievedData retrieve(int batchSize) override;
};

#endif
