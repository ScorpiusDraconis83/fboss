// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const WeutilInfo info) : info_(info) {}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getInfo() {
  return eepromParser.getEeprom(info_.eepromPath, info_.offset);
}

void WeutilImpl::printInfo() {
  EepromEntry info = eepromParser.getEeprom(info_.eepromPath, info_.offset);
  std::cout << "Wedge EEPROM : " << info_.eepromPath << std::endl;
  for (const auto& item : info) {
    std::cout << item.first << ": " << item.second << std::endl;
  }
  return;
}

void WeutilImpl::printInfoJson() {
  // Use getInfo, then go thru our table to generate JSON in order
  EepromEntry info = eepromParser.getEeprom(info_.eepromPath, info_.offset);
  int vectorSize = info.size();
  int cursor = 0;
  // Manually create JSON object without using folly, so that this code
  // will be ported to BMC later
  // Print the first part of the JSON - fixed entry to make a JSON
  std::cout << "{";
  std::cout << "\"Information\": {";
  // Print the second part of the JSON, the dynamic entry
  for (auto [key, value] : info) {
    // CRC16 is not needed in JSON output
    if (key == "CRC16") {
      continue;
    }
    std::cout << "\"" << key << "\": ";
    std::cout << "\"" << value << "\"";
    if (cursor++ != vectorSize - 1) {
      std::cout << ", ";
    }
  }

  // Finally, print the third part of the JSON - fixed entry
  std::cout << "}, \"Actions\": [], \"Resources\": []";
  std::cout << "}" << std::endl;
}

std::vector<std::string> WeutilImpl::getModuleNames() {
  return info_.modules;
}

void WeutilImpl::printUsage() {
  std::cout
      << "weutil [--h] [--json] [--eeprom module_name] [--path absolute_path_to_eeprom]"
      << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
  std::cout << "    weutil --path /sys/bus/i2c/drivers/at24/6-0051/eeprom"
            << std::endl;
  std::cout << std::endl;
  auto moduleNames = getModuleNames();
  std::cout << "Module names are : ";
  for (auto module : moduleNames) {
    std::cout << module << " ";
  }
  std::cout << std::endl;
  std::cout
      << "Note that supplying --path option will override any module name or config."
      << std::endl;
  XLOG(INFO) << "printUSage";
}

} // namespace facebook::fboss::platform
