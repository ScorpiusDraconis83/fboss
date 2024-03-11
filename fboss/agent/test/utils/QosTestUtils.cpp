// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/QosTestUtils.h"
#include <folly/IPAddressV4.h>

#include "fboss/agent/state/Port.h"
#include "fboss/lib/CommonUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

namespace {
template <typename AddrT>
void setDisableTTLDecrementOnNeighnor(
    std::shared_ptr<SwitchState>* state,
    InterfaceID intfID,
    AddrT ip) {
  auto intf = (*state)->getInterfaces()->getNodeIf(intfID);
  auto vlan = (*state)->getVlans()->getNodeIf(intf->getVlanID())->modify(state);

  if (vlan->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        vlan->template getNeighborEntryTable<AddrT>()->modify(&vlan, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  } else if (intf->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        intf->template getNeighborEntryTable<AddrT>()->modify(intfID, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  }
}
} // namespace

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    InterfaceID intfID,
    const folly::IPAddress& nhop) {
  // TODO: get the scope of the interface from the state and verify it is
  // supported on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    throw FbossError("Next hop ttl decrement is not supported on this ASIC");
  }
  auto newState = state->clone();
  for (auto iter : std::as_const(*newState->getFibs())) {
    auto fibMap = iter.second;
    auto fibContainer = fibMap->getFibContainerIf(routerId);
    if (!fibContainer) {
      continue;
    }
    auto v4Fib = fibContainer->getFibV4()->modify(routerId, &newState);
    auto v6Fib = fibContainer->getFibV6()->modify(routerId, &newState);
    v4Fib->disableTTLDecrement(nhop);
    v6Fib->disableTTLDecrement(nhop);
  }

  if (nhop.isV4()) {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV4());
  } else {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV6());
  }
  return newState;
}

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& port) {
  // TODO: get the scope of the port from the state and verify it is supported
  // on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    throw FbossError("port ttl decrement is not supported on this ASIC");
  }
  if (!port.isPhysicalPort()) {
    throw FbossError("Cannot ttl decrement for non-physical port");
  }
  auto newState = state->clone();
  auto phyPort =
      state->getPorts()->getNodeIf(port.phyPortID())->modify(&newState);
  phyPort->setTTLDisableDecrement(true);
  return newState;
}

void disableTTLDecrements(TestEnsembleIf* hw, const PortDescriptor& port) {
  hw->applyNewState(
      [asicTable = hw->getHwAsicTable(),
       port](const std::shared_ptr<SwitchState>& state) {
        auto newState = disableTTLDecrement(asicTable, state, port);
        return newState;
      },
      "Disable TTL decrements on port");
}

void disableTTLDecrements(
    TestEnsembleIf* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop) {
  hw->applyNewState(
      [=, asicTable = hw->getHwAsicTable()](
          const std::shared_ptr<SwitchState>& state) {
        auto newState =
            disableTTLDecrement(asicTable, state, routerId, intf, nhop);
        return newState;
      },
      "Disable TTL decrements on next hop: " + nhop.str());
}

bool queueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::PortID egressPort) {
  auto queuePacketsBefore = portStatsBefore.queueOutPackets_()->find(queueId) !=
          portStatsBefore.queueOutPackets_()->end()
      ? portStatsBefore.queueOutPackets_()->find(queueId)->second
      : 0;
  int64_t queuePacketsAfter = 0;
  auto latestPortStats = sw->getHwPortStats({egressPort});
  if (latestPortStats.find(egressPort) != latestPortStats.end()) {
    auto portStatsAfter = latestPortStats[egressPort];
    if (portStatsAfter.queueOutPackets_()->find(queueId) !=
        portStatsAfter.queueOutPackets_()->end()) {
      queuePacketsAfter = portStatsAfter.queueOutPackets_()[queueId];
    }
  }
  // Note, on some platforms, due to how loopbacked packets are pruned
  // from being broadcast, they will appear more than once on a queue
  // counter, so we can only check that the counter went up, not that it
  // went up by exactly one.
  XLOG(DBG2) << "Port ID: " << egressPort << " queue: " << queueId
             << " queuePacketsBefore " << queuePacketsBefore
             << " queuePacketsAfter " << queuePacketsAfter;
  return queuePacketsAfter > queuePacketsBefore;
}

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::PortID egressPort) {
  WITH_RETRIES({
    EXPECT_EVENTUALLY_TRUE(queueHit(portStatsBefore, queueId, sw, egressPort));
  });
}

} // namespace facebook::fboss::utility
