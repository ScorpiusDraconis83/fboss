/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"

namespace facebook::fboss {

class BeasAsic;

class SaiKametPlatform : public SaiBcmPlatform {
 public:
  explicit SaiKametPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiKametPlatform() override;
  HwAsic* getAsic() const override;

  uint32_t numLanesPerCore() const override {
    return 1;
  }

  uint32_t numCellsAvailable() const override {
    return 130665;
  }

  bool isSerdesApiSupported() const override {
    /*
     * Ungate this when port profile mappings are done.
     * Serdes settings require platform port profiles
     */
    return false;
  }

  void initLEDs() override;

  std::vector<PortID> getAllPortsInGroup(PortID portID) const override {
    return {};
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {};
  }

  bool supportInterfaceType() const override {
    return true;
  }

  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override {
    return std::nullopt;
  }

 private:
  std::unique_ptr<BeasAsic> asic_;
};

} // namespace facebook::fboss
