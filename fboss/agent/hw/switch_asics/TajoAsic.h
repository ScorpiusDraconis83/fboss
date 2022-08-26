// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class TajoAsic : public HwAsic {
 public:
  using HwAsic::HwAsic;
  std::string getVendor() const override {
    return "tajo";
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  uint64_t getMMUSizeBytes() const override {
    return 108 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 9;
  }
  AsicVendor getAsicVendor() const override {
    return HwAsic::AsicVendor::ASIC_VENDOR_TAJO;
  }
  bool scalingFactorBasedDynamicThresholdSupported() const override {
    return false;
  }
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor /* scalingFactor */) const override {
    throw FbossError("Dynamic buffer threshold unsupported!");
  }
};

} // namespace facebook::fboss
