/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/CommonUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

DECLARE_bool(sai_user_defined_trap);

namespace {
constexpr auto kGetQueueOutPktsRetryTimes = 5;
/**
 * Link-local multicast network
 */
const auto kIPv6LinkLocalMcastAbsoluteAddress = folly::IPAddressV6("ff02::0");
const auto kIPv6LinkLocalMcastAddress = folly::IPAddressV6("ff02::5");
// Link-local unicast network
const auto kIPv6LinkLocalUcastAddress = folly::IPAddressV6("fe80::2");

const auto kMcastMacAddress = folly::MacAddress("01:05:0E:01:02:03");

const auto kNetworkControlDscp = 48;

const auto kDhcpV6AllRoutersIp = folly::IPAddressV6("ff02::1:2");
const auto kDhcpV6McastMacAddress = folly::MacAddress("33:33:00:01:00:02");
const auto kDhcpV6ServerGlobalUnicastAddress =
    folly::IPAddressV6("2401:db00:eef0:a67::1");

using TestTypes =
    ::testing::Types<facebook::fboss::PortID, facebook::fboss::AggregatePortID>;
} // unnamed namespace

namespace facebook::fboss {

template <typename TestType>
class AgentCoppTest : public AgentHwTest {
  void SetUp() override {
    FLAGS_sai_user_defined_trap = true;
    AgentHwTest::SetUp();
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_FORWARDING};
  }

 protected:
  static constexpr auto isTrunk = std::is_same_v<TestType, AggregatePortID>;

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    if (isTrunk) {
      return getTrunkInitialConfig(ensemble);
    }

    auto asic = utility::getFirstAsic(ensemble.getSw());
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    utility::addOlympicQosMaps(cfg, asic);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, asic, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, asic, ensemble.isSai());
    return cfg;
  }

  cfg::SwitchConfig getTrunkInitialConfig(const AgentEnsemble& ensemble) const {
    auto asic = utility::getFirstAsic(ensemble.getSw());
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, asic, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, asic, ensemble.isSai());
    utility::addAggPort(
        1,
        {ensemble.masterLogicalPortIds()[0],
         ensemble.masterLogicalPortIds()[1]},
        &cfg);
    return cfg;
  }

  void setup() {
    // COPP is part of initial config already
    if constexpr (isTrunk) {
      applyNewState([this](std::shared_ptr<SwitchState> /*in*/) {
        return utility::enableTrunkPorts(getProgrammedState());
      });
    }
  }

  folly::IPAddress getInSubnetNonSwitchIP() const {
    auto configIntf = initialConfig(*getAgentEnsemble()).interfaces()[0];
    auto ipAddress = configIntf.ipAddresses()[0];
    return folly::IPAddress::createNetwork(ipAddress, -1, true).first;
  }

  void sendTcpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    utility::sendTcpPkts(
        getSw(),
        numPktsToSend,
        vlanId,
        dstMac ? *dstMac : intfMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0],
        trafficClass,
        payload);
  }

  // With onePortPerInterfaceConfig, we have a large (~200) number of
  // interfaces. Thus, if we send packet for every interface, the test takes
  // really long (5+ mins) to complete), and does not really offer additional
  // coverage. Thus, pick one IPv4 and IPv6 address and test.
  std::vector<std::string> getIpAddrsToSendPktsTo() const {
    std::set<std::string> ips;
    auto addV4AndV6 = [&](const auto& ipAddrs) {
      auto ipv4Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV4();
          });
      auto ipv6Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV6();
          });
      ips.insert(*ipv4Addr);
      ips.insert(*ipv6Addr);
    };
    addV4AndV6(*(this->initialConfig(*getAgentEnsemble())
                     .interfaces()[0]
                     .ipAddresses()));

    auto switchId = utility::getFirstSwitchId(getAgentEnsemble()->getSw());
    if (switchId) {
      auto dsfNode = getProgrammedState()->getDsfNodes()->getNodeIf(switchId);
      if (dsfNode) {
        auto loopbackIps = dsfNode->getLoopbackIps();
        std::vector<std::string> subnets;
        std::for_each(
            loopbackIps->begin(), loopbackIps->end(), [&](const auto& ip) {
              subnets.push_back(**ip);
            });
        addV4AndV6(subnets);
      }
    }

    return std::vector<std::string>{ips.begin(), ips.end()};
  }

  void sendPkt(
      std::unique_ptr<TxPacket> pkt,
      bool outOfPort,
      bool snoopAndVerify = false) {
    XLOG(DBG2) << "Packet Dump::"
               << folly::hexDump(pkt->buf()->data(), pkt->buf()->length());

    auto ethFrame = utility::makeEthFrame(*pkt, true /*skipTtlDecrement*/);
    if (outOfPort) {
      getSw()->sendPacketOutOfPortAsync(
          std::move(pkt),
          masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(pkt));
    }
    if (snoopAndVerify) {
      // TODO - Add support for snoop and verify
    }
  }

  void sendTcpPktAndVerifyCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt,
      bool expectQueueHit = true,
      bool outOfPort = true) {
    const auto kNumPktsToSend = 1;
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto destinationMac =
        dstMac.value_or(utility::getFirstInterfaceMac(getProgrammedState()));
    auto sendAndInspect = [=, this]() {
      auto pkt = utility::makeTCPTxPacket(
          getSw(),
          vlanId,
          destinationMac,
          dstIpAddress,
          l4SrcPort,
          l4DstPort,
          trafficClass,
          payload);
      sendPkt(std::move(pkt), outOfPort, expectQueueHit /*snoopAndVerify*/);
    };
    utility::sendPktAndVerifyCpuQueue(
        getSw(), queueId, sendAndInspect, expectQueueHit ? kNumPktsToSend : 0);
  }

  uint64_t getQueueOutPacketsWithRetry(
      int queueId,
      int retryTimes,
      uint64_t expectedNumPkts,
      int postMatchRetryTimes = 2) {
    uint64_t outPkts = 0;
    auto switchId = utility::getFirstSwitchId(getSw());
    do {
      for (auto i = 0;
           i <= utility::getCoppHighPriQueueId(utility::getFirstAsic(getSw()));
           i++) {
        auto qOutPkts = utility::getCpuQueueInPackets(getSw(), switchId, i);
        XLOG(DBG2) << "QueueID: " << i << " qOutPkts: " << qOutPkts;
      }

      outPkts = utility::getCpuQueueInPackets(getSw(), switchId, queueId);
      if (retryTimes == 0 || (outPkts >= expectedNumPkts)) {
        break;
      }

      /*
       * Post warmboot, the packet always gets processed by the right CPU
       * queue (as per ACL/rxreason etc.) but sometimes it is delayed.
       * Retrying a few times to avoid test noise.
       */
      XLOG(DBG0) << "Retry...";
      /* sleep override */
      sleep(1);
    } while (retryTimes-- > 0);

    while ((outPkts == expectedNumPkts) && postMatchRetryTimes--) {
      outPkts = utility::getCpuQueueInPackets(getSw(), switchId, queueId);
    }

    return outPkts;
  }

  void sendUdpPkt(
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      uint8_t ttl,
      bool outOfPort,
      bool expectPktTrap) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    // arbit
    const auto srcIp =
        folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        srcIp,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        0 /* dscp */,
        ttl);

    XLOG(DBG2) << "UDP packet Dump::"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());
    sendPkt(std::move(txPacket), outOfPort, expectPktTrap /*snoopAndVerify*/);
  }

  void sendUdpPktAndVerify(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      bool expectPktTrap = true,
      const int ttl = 255,
      bool outOfPort = false) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    auto expectedPktDelta = expectPktTrap ? 1 : 0;
    sendUdpPkt(
        dstIpAddress, l4SrcPort, l4DstPort, ttl, outOfPort, expectPktTrap);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendEthPkts(
      int numPktsToSend,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          intfMac,
          dstMac ? *dstMac : intfMac,
          etherType,
          payload);
      sendPkt(std::move(txPacket), true /*outOfPort*/, true /*snoopAndVerify*/);
    }
  }

  void sendPktAndVerifyEthPacketsCpuQueue(
      int queueId,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    static auto payload = std::vector<uint8_t>(256, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    sendEthPkts(1, etherType, dstMac, payload);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac="
               << (dstMac ? (*dstMac).toString()
                          : getLocalMacAddress().toString())
               << ". Ethertype=" << std::hex << int(etherType)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(1, afterOutPkts - beforeOutPkts);
  }
};

TYPED_TEST_SUITE(AgentCoppTest, TestTypes);

TYPED_TEST(AgentCoppTest, LocalDstIpBgpPortToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Make sure all dstip(=interfaces local ips) + BGP port packets send to
    // cpu high priority queue
    enum SRC_DST { SRC, DST };
    auto asic = utility::getFirstAsic(this->getSw());
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      for (int dir = 0; dir <= DST; dir++) {
        XLOG(DBG2) << "Send Pkt to: " << ipAddress
                   << " dir: " << (dir == DST ? " DST" : " SRC");
        this->sendTcpPktAndVerifyCpuQueue(
            utility::getCoppHighPriQueueId(asic),
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            dir == SRC ? utility::kBgpPort : utility::kNonSpecialPort1,
            dir == DST ? utility::kBgpPort : utility::kNonSpecialPort1);
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, LocalDstIpNonBgpPortToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastTxFromCpu) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Intent of this test is to verify that
    // link local ipv6 address is not looped back when sent from CPU
    this->sendUdpPktAndVerify(
        utility::kCoppMidPriQueueId,
        folly::IPAddressV6("ff02::1"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        false /* expectPktTrap */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalUcastToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Device link local unicast address should use mid-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, getLocalMacAddress());

      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
    // Non device link local unicast address should also use mid-pri queue
    {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, SlowProtocolsMacToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
        LACPDU::kSlowProtocolsDstMac());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, EapolToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        facebook::fboss::ETHERTYPE::ETHERTYPE_EAPOL,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, DstIpNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non local dst ip with kNetworkControlDscp should not hit high pri queue
    // (since it won't even trap to cpu)
    this->sendTcpPktAndVerifyCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddress("2::2"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        kNetworkControlDscp,
        std::nullopt,
        false /*expectQueueHit*/);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalUcastIpNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Device link local unicast address + kNetworkControlDscp should use
    // high-pri queue
    {
      XLOG(DBG2) << "send device link local packet";
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, utility::kLocalCpuMac());

      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non device link local unicast address + kNetworkControlDscp dscp should
    // also use high-pri queue
    {
      XLOG(DBG2) << "send non-device link local packet";
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

/*
 * Testcase to test that link local ucast packets from cpu port does not get
 * copied back to cpu. The test does the following
 * 1. copp acl to match link local ucast address and cpu srcPort is created as a
 * part of setup
 * 2. Sends a link local unicast packet through CPU PIPELINE_LOOKUP.
 * 3. Packet hits the newly created acl(and so does not get forwarded to cpu).
 * It goes out through the front port and loops back in.
 * 4. On getting received, the packet bypasses the newly created acl(since src
 * port is no longer cpu port) and hits the acl for regular link local ucast
 * packets. This gets forwarded to cpu queue 9
 */
TYPED_TEST(AgentCoppTest, CpuPortIpv6LinkLocalUcastIp) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    auto nbrLinkLocalAddr = folly::IPAddressV6("fe80:face:b11c::1");
    auto randomMac = folly::MacAddress("00:00:00:00:00:01");
    this->sendTcpPktAndVerifyCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        nbrLinkLocalAddr,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        randomMac,
        kNetworkControlDscp,
        std::nullopt,
        true,
        false /*outOfPort*/);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress,
          kNetworkControlDscp);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
