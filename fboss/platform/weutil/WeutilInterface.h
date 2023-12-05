// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::platform {

class WeutilInterface {
 public:
  WeutilInterface() {}
  virtual void printInfo() = 0;
  virtual void printInfoJson() = 0;
  virtual bool getEepromPath(void) = 0;
  // get weutil info in a vector of pairs, e.g. <"Version", "x"> , etc
  virtual std::vector<std::pair<std::string, std::string>> getInfo(
      const std::string& eeprom = "") = 0;
  virtual ~WeutilInterface() = default;
  virtual void printUsage() = 0;
};

} // namespace facebook::fboss::platform
