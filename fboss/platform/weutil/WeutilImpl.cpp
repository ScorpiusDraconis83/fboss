// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <ios>
#include <unordered_map>

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eeprom, PlainWeutilConfig config)
    : config_(config) {
  XLOG(INFO) << "WeutilImpl: eeprom = " << eeprom;
  initializeFieldDictionaryV3();
  initializeFieldDictionaryV4();
  eepromPath = eeprom;
}

//
//  This method is called inside the constructor to initialize the
//  internal look-up table. The content of this table follows FBOSS
//  EEPROM spec v4.
//
void WeutilImpl::initializeFieldDictionaryV4() {
  //
  // VARIABLE in the offset and length means it's variable, and
  // will be determined by the TLV structure in the EEPROM
  // itself (v4 and above)
  // {Typecode, Name, FieldType, Length, Offset}
  // Note that this is the order that the EEPROM will be printed
  //
  fieldDictionaryV4_.push_back(
      {0, "NA", FIELD_UINT, -1, -1}); // TypeCode 0 is reserved
  fieldDictionaryV4_.push_back(
      {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE});
  fieldDictionaryV4_.push_back(
      {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE});
  fieldDictionaryV4_.push_back(
      {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE});
  fieldDictionaryV4_.push_back(
      {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {8, "Product Production State", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back({9, "Product Version", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back(
      {10, "Product Sub-Version", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back(
      {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE});
  fieldDictionaryV4_.push_back(
      {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {15, "Assembled at", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back({16, "Local MAC", FIELD_MAC, 6, VARIABLE});
  fieldDictionaryV4_.push_back(
      {17, "Extended MAC Base", FIELD_MAC, 6, VARIABLE});
  fieldDictionaryV4_.push_back(
      {18, "Extended MAC Address Size", FIELD_UINT, 2, VARIABLE});
  fieldDictionaryV4_.push_back(
      {19, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back({250, "CRC16", FIELD_HEX, 2, VARIABLE});
}

// Same as above, but for EEPROM V3.
void WeutilImpl::initializeFieldDictionaryV3() {
  //
  // VARIABLE in the offset and length means it's variable, and
  // will be determined by the TLV structure in the EEPROM
  // itself (v4 and above)
  // {Typecode, Name, FieldType, Length, Offset}
  // Note that this is the order that the EEPROM will be printed
  //
  fieldDictionaryV3_.push_back(
      {0, "Version", FIELD_UINT, 1, 2}); // TypeCode 0 is reserved
  fieldDictionaryV3_.push_back({1, "Product Name", FIELD_STRING, 20, 3});
  fieldDictionaryV3_.push_back({2, "Product Part Number", FIELD_STRING, 8, 23});
  fieldDictionaryV3_.push_back(
      {3, "System Assembly Part Number", FIELD_STRING, 12, 31});
  fieldDictionaryV3_.push_back(
      {4, "Meta PCBA Part Number", FIELD_STRING, 12, 43});
  fieldDictionaryV3_.push_back(
      {5, "Meta PCB Part Number", FIELD_STRING, 12, 55});
  fieldDictionaryV3_.push_back(
      {6, "ODM/JDM PCBA Part Number", FIELD_STRING, 13, 67});
  fieldDictionaryV3_.push_back(
      {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, 13, 80});
  fieldDictionaryV3_.push_back(
      {8, "Product Production State", FIELD_UINT, 1, 93});
  fieldDictionaryV3_.push_back({9, "Product Version", FIELD_UINT, 1, 94});
  fieldDictionaryV3_.push_back({10, "Product Sub-Version", FIELD_UINT, 1, 95});
  fieldDictionaryV3_.push_back(
      {11, "Product Serial Number", FIELD_STRING, 13, 96});
  fieldDictionaryV3_.push_back(
      {12, "Product Asset Tag", FIELD_STRING, 12, 109});
  fieldDictionaryV3_.push_back(
      {12, "System Manufacturer", FIELD_STRING, 8, 121});
  fieldDictionaryV3_.push_back(
      {13, "System Manufacturing Date", FIELD_DATE, 4, 129});
  fieldDictionaryV3_.push_back({14, "PCB Manufacturer", FIELD_STRING, 8, 133});
  fieldDictionaryV3_.push_back({15, "Assembled at", FIELD_STRING, 8, 141});
  fieldDictionaryV3_.push_back({16, "Local MAC", FIELD_LEGACY_MAC, 12, 149});
  fieldDictionaryV3_.push_back(
      {17, "Extended MAC Base", FIELD_LEGACY_MAC, 12, 161});
  fieldDictionaryV3_.push_back(
      {18, "Extended MAC Address Size", FIELD_UINT, 2, 173});
  fieldDictionaryV3_.push_back(
      {19, "Location on Fabric", FIELD_STRING, 20, 175});
  fieldDictionaryV3_.push_back({250, "CRC8", FIELD_HEX, 1, 195});
}

/*
 * Helper function, given the eeprom path, read it and store the blob
 * to the char array output
 */
int WeutilImpl::loadEeprom(
    const std::string& eeprom,
    unsigned char* output,
    int offset,
    int max) {
  // Declare buffer, and fill it up with 0s
  int fileSize = 0;
  std::ifstream file(eeprom, std::ios::binary);
  int readCount = 0;
  // First, detect EEPROM size, upto 2048B only
  try {
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    fileSize = fileSize > max ? max : fileSize;
  } catch (std::exception& ex) {
    std::cout << "Failed to detect EEPROM size (" << eeprom
              << "): " << ex.what() << std::endl;
  }
  if (fileSize < 0) {
    std::cout << "EEPROM (" << eeprom << ") does not exist, or is empty!"
              << std::endl;
    throw std::runtime_error("Unable to read EEPROM.");
  }
  if (offset > fileSize) {
    throw std::runtime_error("Offset exceeds EEPROM size.");
  }
  // Now, read the eeprom
  try {
    file.seekg(offset, std::ios::beg);
    file.read((char*)&output[0], (fileSize - offset));
    readCount = (int)file.gcount();
    file.close();
  } catch (std::exception& ex) {
    std::cout << "Failed to read EEPROM contents " << ex.what() << std::endl;
    readCount = 0;
  }
  return readCount;
}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getInfo(
    const std::string& eeprom) {
  std::vector<std::pair<std::string, std::string>> info;

  XLOG(INFO) << "getInfo: eeprom = " << eeprom;
  return info;
}

// Parse Uint field
std::string WeutilImpl::parseUint(int len, unsigned char* ptr) {
  // For now, we only support up to 4 Bytes of data
  if (len > 4) {
    throw std::runtime_error("Unsigned int can be up to 4 bytes only.");
  }
  unsigned int readVal = 0;
  // Values in the EEPROM is little endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[cursor];
    cursor -= 1;
  }
  return std::to_string(readVal);
}

// Parse String field
std::string WeutilImpl::parseString(int len, unsigned char* ptr) {
  // std::string only takes char *, not unsigned char*
  // but the data we want to parse is alphanumeric string
  // whose ASCII value is less than 127. So it is safe to cast
  // unsigned char to char here.
  return std::string((const char*)ptr, len);
}

// For EEPROM V4, Parse MAC with the format XX:XX:XX:XX:XX:XX
std::string WeutilImpl::parseMac(int len, unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  int juice = 0;
  while ((juice < len) && (ptr[juice] != 0)) {
    unsigned int val = ptr[juice];
    std::ostringstream ss;
    ss << std::hex << val;
    std::string strElement = ss.str();
    // Pad 0 if the hex value is only 1 digit. Also,
    // add ':' between 2 hex digits except for the last element
    strElement =
        (val < 16 ? "0" : "") + strElement + (juice != len - 1 ? ":" : "");
    retVal += strElement;
    juice = juice + 1;
  }
  return retVal;
}

// For EEPROM V3, MAC is represented as XXXXXXXXXXXX. Therefore,
// parse the MAC into the human readable format as XX:XX:XX:XX:XX:XX
std::string WeutilImpl::parseLegacyMac(int len, unsigned char* ptr) {
  std::string retVal = "";
  if (len != 12) {
    throw std::runtime_error("Legacy(v3) MAC field must be 12 Bytes Long!");
  }
  for (int i = 0; i < 12; i++) {
    retVal += (ptr[i]);
    if (((i % 2) == 1) && (i != 11)) {
      retVal += ":";
    }
  }
  return retVal;
}

std::string WeutilImpl::parseDate(int len, unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  if (len != 4) {
    throw std::runtime_error("Date field must be 4 Bytes Long!");
  }
  unsigned int year = (unsigned int)ptr[1] + (unsigned int)ptr[0];
  unsigned int month = (unsigned int)ptr[2];
  unsigned int day = (unsigned int)ptr[3];
  std::string yearString = std::to_string(year % 100);
  std::string monthString = std::to_string(month);
  std::string dayString = std::to_string(day);
  yearString = (yearString.length() == 1 ? "0" : "") + yearString;
  monthString = (monthString.length() == 1 ? "0" : "") + monthString;
  dayString = (dayString.length() == 1 ? "0" : "") + dayString;
  return monthString + "-" + dayString + "-" + yearString;
}

std::string WeutilImpl::parseHex(int len, unsigned char* ptr) {
  std::stringstream stream;
  stream << std::hex << std::stoi(parseString(len, ptr));
  return stream.str();
}

void WeutilImpl::printInfo() {
  XLOG(INFO) << "printInfo";
}

void WeutilImpl::printInfoJson() {
  XLOG(INFO) << "printInfoJson";
}

bool WeutilImpl::verifyOptions() {
  // If eeprom is in fact the FRU name, replace this name with the actual path
  // If it's not, treat it as the path to the eeprom file / sysfs
  std::string lEeprom = eepromPath;
  std::transform(lEeprom.begin(), lEeprom.end(), lEeprom.begin(), ::tolower);
  if (config_.configEntry.find(lEeprom) != config_.configEntry.end()) {
    eepromPath = get<0>(config_.configEntry[lEeprom]);
  } else {
    throw std::runtime_error("Invalid EEPROM Name.");
  }
  return true;
}
void WeutilImpl::printUsage() {
  std::cout << "weutil [--h] [--json] [--eeprom module_name]" << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
  std::cout << std::endl;
  std::cout << "Module names are : ";
  for (auto item : config_.configEntry) {
    std::cout << item.first << " ";
  }
  std::cout << std::endl;
  XLOG(INFO) << "printUSage";
}

} // namespace facebook::fboss::platform
