// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DataStore.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {

DataStore::DataStore(const PlatformConfig& config) : platformConfig_(config) {}

uint16_t DataStore::getI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName) const {
  auto it = i2cBusNums_.find(std::make_pair(std::nullopt, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  }
  it = i2cBusNums_.find(std::make_pair(slotPath, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  }
  throw std::runtime_error(fmt::format(
      "Could not find bus number for {} at {}",
      pmUnitScopeBusName,
      slotPath ? *slotPath : "CPU"));
}

void DataStore::updateI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName,
    uint16_t busNum) {
  XLOG(INFO) << fmt::format(
      "Updating bus {} in {} to bus number {} (i2c-{})",
      pmUnitScopeBusName,
      slotPath ? fmt::format("SlotPath {}", *slotPath) : "Global Scope",
      busNum,
      busNum);
  i2cBusNums_[std::make_pair(slotPath, pmUnitScopeBusName)] = busNum;
}

PmUnitInfo DataStore::getPmUnitInfo(const std::string& slotPath) const {
  if (slotPathToPmUnitInfo.find(slotPath) != slotPathToPmUnitInfo.end()) {
    return slotPathToPmUnitInfo.at(slotPath);
  }
  throw std::runtime_error(
      fmt::format("Could not find PmUnit at {}", slotPath));
}

bool DataStore::hasPmUnit(const std::string& slotPath) const {
  return slotPathToPmUnitInfo.find(slotPath) != slotPathToPmUnitInfo.end();
}

std::string DataStore::getSysfsPath(const std::string& devicePath) const {
  auto itr = pciSubDevicePathToSysfsPath_.find(devicePath);
  if (itr != pciSubDevicePathToSysfsPath_.end()) {
    return itr->second;
  }
  throw std::runtime_error(
      fmt::format("Could not find SysfsPath for {}", devicePath));
}

void DataStore::updateSysfsPath(
    const std::string& devicePath,
    const std::string& sysfsPath) {
  XLOG(INFO) << fmt::format(
      "Updating SysfsPath for {} to {}", devicePath, sysfsPath);
  pciSubDevicePathToSysfsPath_[devicePath] = sysfsPath;
}

bool DataStore::hasSysfsPath(const std::string& devicePath) const {
  return pciSubDevicePathToSysfsPath_.find(devicePath) !=
      pciSubDevicePathToSysfsPath_.end();
}

std::string DataStore::getCharDevPath(const std::string& devicePath) const {
  auto itr = pciSubDevicePathToCharDevPath_.find(devicePath);
  if (itr != pciSubDevicePathToCharDevPath_.end()) {
    return itr->second;
  }
  throw std::runtime_error(
      fmt::format("Could not find CharDevPath for {}", devicePath));
}

void DataStore::updateCharDevPath(
    const std::string& devicePath,
    const std::string& charDevPath) {
  XLOG(INFO) << fmt::format(
      "Updating CharDevPath for {} to {}", devicePath, charDevPath);
  pciSubDevicePathToCharDevPath_[devicePath] = charDevPath;
}

void DataStore::updatePmUnitInfo(
    const std::string& slotPath,
    const std::string& pmUnitName,
    std::optional<int> productSubVersion) {
  XLOG(INFO) << fmt::format(
      "At SlotPath {}, updating to PmUnit {} {}",
      slotPath,
      pmUnitName,
      productSubVersion
          ? fmt::format("ProductSubVersion {}", *productSubVersion)
          : "");
  PmUnitInfo pmUnitInfo;
  pmUnitInfo.name() = pmUnitName;
  if (productSubVersion) {
    pmUnitInfo.version() = PmUnitVersion();
    pmUnitInfo.version()->productSubVersion() = *productSubVersion;
  }
  slotPathToPmUnitInfo[slotPath] = pmUnitInfo;
}

PmUnitConfig DataStore::resolvePmUnitConfig(const std::string& slotPath) const {
  if (slotPathToPmUnitInfo.find(slotPath) == slotPathToPmUnitInfo.end()) {
    throw std::runtime_error(
        fmt::format("Unable to resolve PmUnitInfo for {}", slotPath));
  }
  const auto& pmUnitInfo = slotPathToPmUnitInfo.at(slotPath);
  const auto& pmUnitName = *pmUnitInfo.name();
  const auto& version = pmUnitInfo.version();
  if (!version) {
    XLOG(INFO) << fmt::format(
        "Resolved {} to default PmUnitConfig of {}. No ProductSubversion was "
        "read from IDPROM at the slotPath.",
        slotPath,
        pmUnitName);
    return platformConfig_.pmUnitConfigs()->at(pmUnitName);
  }
  auto productSubVersion = *version->productSubVersion();
  if (platformConfig_.versionedPmUnitConfigs()->contains(pmUnitName)) {
    for (const auto& versionedPmUnitConfig :
         platformConfig_.versionedPmUnitConfigs()->at(pmUnitName)) {
      if (*versionedPmUnitConfig.productSubVersion() == productSubVersion) {
        XLOG(INFO) << fmt::format(
            "Resolved {} to PmUnitConfig of {} with ProductSubVersion {}",
            slotPath,
            pmUnitName,
            productSubVersion);
        return *versionedPmUnitConfig.pmUnitConfig();
      }
    }
  }
  XLOG(INFO) << fmt::format(
      "Resolved {} to default PmUnitConfig of {}. No versioned config for "
      "ProductSubVersion {}",
      slotPath,
      pmUnitName,
      productSubVersion);
  return platformConfig_.pmUnitConfigs()->at(pmUnitName);
}
} // namespace facebook::fboss::platform::platform_manager
