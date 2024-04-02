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
#include <memory>

#include "fboss/agent/mnpu/SplitAgentThriftSyncerClient.h"

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <string>

#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"

namespace facebook::fboss {

class HwSwitch;
class FdbEventSyncer;
class LinkActiveEventSyncer;
class LinkChangeEventSyncer;
class RxPktEventSyncer;
class TxPktEventSyncer;
class OperDeltaSyncer;
class HwSwitchStatsSinkClient;

class SplitAgentThriftSyncer : public HwSwitchCallback {
 public:
  SplitAgentThriftSyncer(
      HwSwitch* hw,
      uint16_t serverPort,
      SwitchID switchId,
      uint16_t switchIndex);
  ~SplitAgentThriftSyncer() override;

  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus =
          std::nullopt) override;
  void linkActiveStateChanged(
      const std::map<PortID, bool>& port2IsActive) override;
  void linkConnectivityChanged(
      const std::map<PortID, multiswitch::FabricConnectivityDelta>&
          port2OldAndNewConnectivity) override;
  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  void exitFatal() const noexcept override;
  void pfcWatchdogStateChanged(const PortID& port, const bool deadlock)
      override;
  void registerStateObserver(StateObserver* observer, const std::string& name)
      override;
  void unregisterStateObserver(StateObserver* observer) override;

  void start();
  void stop();
  void updateHwSwitchStats(multiswitch::HwSwitchStats stats);

 private:
  std::shared_ptr<folly::ScopedEventBaseThread> retryThread_;
  SwitchID switchId_;
  std::unique_ptr<LinkChangeEventSyncer> linkChangeEventSinkClient_;
  std::unique_ptr<TxPktEventSyncer> txPktEventStreamClient_;
  std::unique_ptr<OperDeltaSyncer> operDeltaClient_;
  std::unique_ptr<FdbEventSyncer> fdbEventSinkClient_;
  std::unique_ptr<RxPktEventSyncer> rxPktEventSinkClient_;
  std::unique_ptr<HwSwitchStatsSinkClient> hwSwitchStatsSinkClient_;
  bool isRunning_{false};
};
} // namespace facebook::fboss
