/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss::utility {

namespace {

constexpr auto kNumPortPerCore = 10;
// 0: CPU port, 1: gloabl rcy port, 2-5: local recycle port, 6: eventor port,
// 7: mgm port, 8-43 front panel nif
constexpr auto kRemoteSysPortOffset = 7;
constexpr auto kNumRdsw = 128;
constexpr auto kNumEdsw = 16;
constexpr auto kNumRdswSysPort = 44;
constexpr auto kNumEdswSysPort = 26;
constexpr auto kJ2NumSysPort = 20;

int getPerNodeSysPorts(const HwAsic* asic, int remoteSwitchId) {
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
    return kJ2NumSysPort;
  }
  if (remoteSwitchId < kNumRdsw * asic->getNumCores()) {
    return kNumRdswSysPort;
  }
  return kNumEdswSysPort;
}
} // namespace

int getDsfNodeCount(const HwAsic* asic) {
  return asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2
      ? kNumRdsw
      : kNumRdsw + kNumEdsw;
}

std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteDsfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes) {
  CHECK(!curDsfNodes.empty());
  auto dsfNodes = curDsfNodes;
  const auto& firstDsfNode = dsfNodes.begin()->second;
  CHECK(firstDsfNode.systemPortRange().has_value());
  CHECK(firstDsfNode.nodeMac().has_value());
  folly::MacAddress mac(*firstDsfNode.nodeMac());
  auto asic = HwAsic::makeAsic(
      *firstDsfNode.asicType(),
      cfg::SwitchType::VOQ,
      *firstDsfNode.switchId(),
      0,
      *firstDsfNode.systemPortRange(),
      mac,
      std::nullopt);
  int numCores = asic->getNumCores();
  CHECK(
      !numRemoteNodes.has_value() ||
      numRemoteNodes.value() < getDsfNodeCount(asic.get()));
  int totalNodes = numRemoteNodes.has_value() ? numRemoteNodes.value() + 1
                                              : getDsfNodeCount(asic.get());
  int systemPortMin = getPerNodeSysPorts(asic.get(), 1);
  for (int remoteSwitchId = numCores; remoteSwitchId < totalNodes * numCores;
       remoteSwitchId += numCores) {
    cfg::Range64 systemPortRange;
    systemPortRange.minimum() = systemPortMin;
    systemPortRange.maximum() =
        systemPortMin + getPerNodeSysPorts(asic.get(), remoteSwitchId) - 1;
    auto remoteDsfNodeCfg = dsfNodeConfig(
        *asic,
        SwitchID(remoteSwitchId),
        systemPortMin,
        *systemPortRange.maximum());
    dsfNodes.insert({remoteSwitchId, remoteDsfNodeCfg});
    systemPortMin = *systemPortRange.maximum() + 1;
  }
  return dsfNodes;
}

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex,
    int corePortIndex) {
  auto newState = currState->clone();
  const auto& localPorts = newState->getSystemPorts()->cbegin()->second;
  auto localPort = localPorts->cbegin()->second;
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  auto remoteSysPort = std::make_shared<SystemPort>(portId);
  remoteSysPort->setName(folly::to<std::string>(
      "hwTestSwitch", remoteSwitchId, ":eth/", portId, "/1"));
  remoteSysPort->setSwitchId(remoteSwitchId);
  remoteSysPort->setNumVoqs(localPort->getNumVoqs());
  remoteSysPort->setCoreIndex(coreIndex);
  remoteSysPort->setCorePortIndex(corePortIndex);
  remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
  remoteSysPort->resetPortQueues(getDefaultVoqConfig());
  remoteSystemPorts->addNode(remoteSysPort, scopeResolver.scope(remoteSysPort));
  return newState;
}

std::shared_ptr<SwitchState> removeRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    SystemPortID portId) {
  auto newState = currState->clone();
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  remoteSystemPorts->removeNode(portId);
  return newState;
}

std::shared_ptr<SwitchState> addRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    InterfaceID intfId,
    const Interface::Addresses& subnets) {
  auto newState = currState;
  auto newRemoteInterfaces = newState->getRemoteInterfaces()->modify(&newState);
  auto newRemoteInterface = std::make_shared<Interface>(
      intfId,
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("RemoteIntf"),
      folly::MacAddress("c6:ca:2b:2a:b1:b6"),
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);
  newRemoteInterface->setAddresses(subnets);
  newRemoteInterfaces->addNode(
      newRemoteInterface, scopeResolver.scope(newRemoteInterface, newState));
  return newState;
}

std::shared_ptr<SwitchState> removeRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    InterfaceID intfId) {
  auto newState = currState;
  auto newRemoteInterfaces = newState->getRemoteInterfaces()->modify(&newState);
  newRemoteInterfaces->removeNode(intfId);
  return newState;
}

std::shared_ptr<SwitchState> addRemoveRemoteNeighbor(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const folly::IPAddressV6& neighborIp,
    InterfaceID intfID,
    PortDescriptor port,
    bool add,
    std::optional<int64_t> encapIndex) {
  auto outState = currState;
  auto interfaceMap = outState->getRemoteInterfaces()->modify(&outState);
  auto interface = interfaceMap->getNode(intfID)->clone();
  auto ndpTable = interfaceMap->getNode(intfID)->getNdpTable()->clone();
  if (add) {
    const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
    state::NeighborEntryFields ndp;
    ndp.mac() = kNeighborMac.toString();
    ndp.ipaddress() = neighborIp.str();
    ndp.portId() = port.toThrift();
    ndp.interfaceId() = static_cast<int>(intfID);
    ndp.state() = state::NeighborState::Reachable;
    if (encapIndex) {
      ndp.encapIndex() = *encapIndex;
    }
    ndp.isLocal() = false;
    ndpTable->emplace(neighborIp.str(), std::move(ndp));
  } else {
    ndpTable->remove(neighborIp.str());
  }
  interface->setNdpTable(ndpTable->toThrift());
  interfaceMap->updateNode(interface, scopeResolver.scope(interface, outState));
  return outState;
}

std::shared_ptr<SwitchState> setupRemoteIntfAndSysPorts(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const cfg::SwitchConfig& config,
    bool useEncapIndex) {
  auto newState = currState->clone();
  for (const auto& [remoteSwitchId, dsfNode] : *config.dsfNodes()) {
    if (remoteSwitchId == 0) {
      continue;
    }
    CHECK(dsfNode.systemPortRange().has_value());
    const auto minPortID = *dsfNode.systemPortRange()->minimum();
    const auto maxPortID = *dsfNode.systemPortRange()->maximum();
    // 0th port for CPU and 1st port for recycle port
    for (int i = minPortID + kRemoteSysPortOffset; i <= maxPortID; i++) {
      const SystemPortID remoteSysPortId(i);
      const InterfaceID remoteIntfId(i);
      const PortDescriptor portDesc(remoteSysPortId);
      const std::optional<uint64_t> encapEndx =
          useEncapIndex ? std::optional<uint64_t>(0x200001 + i) : std::nullopt;

      // Use subnet 100+(dsfNodeId/256):(dsfNodeId%256):(localIntfId)::1/64
      // and 100+(dsfNodeId/256).(dsfNodeId%256).(localIntfId).1/24
      auto firstOctet = 100 + remoteSwitchId / 256;
      auto secondOctet = remoteSwitchId % 256;
      auto thirdOctet = i - minPortID;
      folly::IPAddressV6 neighborIp(folly::to<std::string>(
          firstOctet, ":", secondOctet, ":", thirdOctet, "::2"));
      newState = addRemoteSysPort(
          newState,
          scopeResolver,
          remoteSysPortId,
          SwitchID(remoteSwitchId),
          (i - minPortID - kRemoteSysPortOffset) / kNumPortPerCore,
          (i - minPortID) % kNumPortPerCore);
      newState = addRemoteInterface(
          newState,
          scopeResolver,
          remoteIntfId,
          {
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ":", secondOctet, ":", thirdOctet, "::1")),
               64},
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ".", secondOctet, ".", thirdOctet, ".1")),
               24},
          });
      newState = addRemoveRemoteNeighbor(
          newState,
          scopeResolver,
          neighborIp,
          remoteIntfId,
          portDesc,
          true /* add */,
          encapEndx);
    }
  }
  return newState;
}

QueueConfig getDefaultVoqConfig() {
  QueueConfig queueCfg;

  auto defaultQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  defaultQueue->setStreamType(cfg::StreamType::UNICAST);
  defaultQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  defaultQueue->setName("default");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(defaultQueue);

  auto rdmaQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(2));
  rdmaQueue->setStreamType(cfg::StreamType::UNICAST);
  rdmaQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  rdmaQueue->setName("rdma");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(rdmaQueue);

  auto monitoringQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(6));
  monitoringQueue->setStreamType(cfg::StreamType::UNICAST);
  monitoringQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  monitoringQueue->setName("monitoring");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(monitoringQueue);

  auto ncQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(7));
  ncQueue->setStreamType(cfg::StreamType::UNICAST);
  ncQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  ncQueue->setName("nc");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(ncQueue);

  return queueCfg;
}

std::optional<uint64_t> getDummyEncapIndex(TestEnsembleIf* ensemble) {
  std::optional<uint64_t> dummyEncapIndex;
  if (ensemble->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    dummyEncapIndex = 0x200001;
  }
  return dummyEncapIndex;
}

// Resolve and return list of remote nhops
boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper) {
  auto remoteSysPorts =
      ensemble->getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
  boost::container::flat_set<PortDescriptor> sysPortDescs;
  std::for_each(
      remoteSysPorts->begin(),
      remoteSysPorts->end(),
      [&sysPortDescs](const auto& idAndPort) {
        sysPortDescs.insert(
            PortDescriptor(static_cast<SystemPortID>(idAndPort.first)));
      });
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(
        in, sysPortDescs, false, getDummyEncapIndex(ensemble));
  });
  return sysPortDescs;
}

} // namespace facebook::fboss::utility
