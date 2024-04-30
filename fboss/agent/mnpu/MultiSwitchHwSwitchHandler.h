#pragma once

#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"

DECLARE_int32(oper_delta_ack_timeout);

namespace facebook::fboss {

class SwSwitch;

class MultiSwitchHwSwitchHandler : public HwSwitchHandler {
 public:
  MultiSwitchHwSwitchHandler(
      const SwitchID& switchId,
      const cfg::SwitchInfo& info,
      SwSwitch* sw);

  virtual ~MultiSwitchHwSwitchHandler() override;

  void exitFatal() const override;

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  void unregisterCallbacks() override;

  void gracefulExit() override;

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  std::optional<uint32_t> getHwLogicalPortId(PortID portID) const override;

  bool transactionsSupported(
      std::optional<cfg::SdkVersion> sdkVersion) const override;

  void switchRunStateChanged(SwitchRunState newState) override;

  // platform access apis
  void onHwInitialized(HwSwitchCallback* callback) override;

  void onInitialConfigApplied(HwSwitchCallback* sw) override;

  void platformStop() override;

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction) override;

  std::pair<fsdb::OperDelta, HwSwitchStateUpdateStatus> stateChanged(
      const fsdb::OperDelta& delta,
      bool transaction,
      const std::shared_ptr<SwitchState>& newState) override;

  std::map<PortID, FabricEndpoint> getFabricConnectivity() const override;

  FabricReachabilityStats getFabricReachabilityStats() const override;

  std::vector<PortID> getSwitchReachability(SwitchID switchId) const override;

  bool needL2EntryForNeighbor(const cfg::SwitchConfig* config) const override;

  multiswitch::StateOperDelta getNextStateOperDelta(
      std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
      int64_t lastUpdateSeqNum) override;

  void notifyHwSwitchDisconnected() override;
  void cancelOperDeltaSync() override;
  HwSwitchOperDeltaSyncState getHwSwitchOperDeltaSyncState() override;

  bool sendPacketOutViaThriftStream(
      std::unique_ptr<TxPacket> pkt,
      std::optional<PortID> portID = std::nullopt,
      std::optional<uint8_t> queue = std::nullopt);

  SwitchRunState getHwSwitchRunState() override;

 private:
  bool checkOperSyncStateLocked(
      HwSwitchOperDeltaSyncState state,
      const std::unique_lock<std::mutex>& /*lock*/) const;
  void setOperSyncStateLocked(
      HwSwitchOperDeltaSyncState state,
      const std::unique_lock<std::mutex>& /*lock*/) {
    operDeltaSyncState_ = state;
  }
  HwSwitchOperDeltaSyncState getOperSyncState() {
    return operDeltaSyncState_;
  }
  /* wait for ack. returns false if cancelled */
  bool waitForOperSyncAck(
      std::unique_lock<std::mutex>& lk,
      uint64_t timeoutInSec);
  /*
   * wait for oper delta to be ready from swswitch.
   * returns false if cancelled or timedout
   */
  bool waitForOperDeltaReady(
      std::unique_lock<std::mutex>& lk,
      uint64_t timeoutInSec);
  void fillMultiswitchOperDelta(
      multiswitch::StateOperDelta& stateDelta,
      const std::shared_ptr<SwitchState>& state,
      const fsdb::OperDelta& delta,
      bool transaction,
      int64_t lastSeqNum);
  void operDeltaAckTimeout();

  SwSwitch* sw_;
  std::condition_variable stateUpdateCV_;
  std::mutex stateUpdateMutex_;
  multiswitch::StateOperDelta* nextOperDelta_{nullptr};
  multiswitch::StateOperDelta* prevOperDeltaResult_{nullptr};
  std::shared_ptr<SwitchState> prevUpdateSwitchState_{nullptr};
  HwSwitchOperDeltaSyncState operDeltaSyncState_{DISCONNECTED};
  int64_t currOperDeltaSeqNum_{0};
  int64_t lastAckedOperDeltaSeqNum_{-1};
  bool operRequestInProgress_{false};
};

} // namespace facebook::fboss
