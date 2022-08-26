// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/BroadcomAsic.h"

namespace facebook::fboss {

class IndusAsic : public BroadcomAsic {
 public:
  using BroadcomAsic::BroadcomAsic;
  bool isSupported(Feature) const override;
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_INDUS;
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::FOURHUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(bool cpu) const override {
    if (cpu) {
      return {cfg::StreamType::MULTICAST};
    } else {
      return {cfg::StreamType::UNICAST};
    }
  }
  int getDefaultNumPortQueues(cfg::StreamType streamType, bool cpu)
      const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 9;
  }
  uint64_t getMMUSizeBytes() const override {
    return 64 * 1024 * 1024;
  }
  uint32_t getMMUCellSize() const {
    return 254;
  }
  uint32_t getMaxMirrors() const override {
    return 4;
  }
  uint64_t getDefaultReservedBytes(cfg::StreamType /*streamType*/, bool cpu)
      const override {
    return cpu ? 1778 : 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    return 160;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 254;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 64;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 254;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 40;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 128;
  }
  uint32_t getMaxEcmpSize() const override {
    return 4096;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 0;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    // Per ITM buffers limits the queue size
    return getMMUSizeBytes() / 2;
  }
  uint32_t getNumCores() const override {
    return 2;
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
