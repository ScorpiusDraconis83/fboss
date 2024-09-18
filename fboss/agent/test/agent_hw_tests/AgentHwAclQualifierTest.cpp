// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

namespace {

using namespace facebook::fboss;

enum class QualifierType : uint8_t {
  LOOKUPCLASS_L2,
  LOOKUPCLASS_NEIGHBOR,
  LOOKUPCLASS_ROUTE,
};

template <typename T, typename U>
void configureQualifier(
    apache::thrift::optional_field_ref<T&> ref,
    bool enable,
    const U& val) {
  if (enable) {
    ref = val;
  } else {
    ref.reset();
  }
}

void configureAllIpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType asicType) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

  if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // TODO(daiweix): remove after J3 ACL supports IP_TYPE
    configureQualifier(acl->ipType(), enable, ipType);
  }
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->srcIp(), enable, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), enable, "2401:db00:3020:70e2:face:0:63:0/64");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);

  } else {
    configureQualifier(acl->srcIp(), enable, "192.168.0.1");
    configureQualifier(acl->dstIp(), enable, "192.168.0.0/24");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
  }
  configureQualifier(
      acl->ipFrag(), enable, cfg::IpFragMatch::MATCH_FIRST_FRAGMENT);
  configureQualifier(acl->dscp(), enable, 0x24);
  configureQualifier(acl->ttl(), enable, ttl);
}

void configureAllTcpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::AsicType /*asicType*/) {
  configureQualifier(acl->l4SrcPort(), enable, 10);
  configureQualifier(acl->l4DstPort(), enable, 20);
  configureQualifier(acl->proto(), enable, 6);
  configureQualifier(acl->tcpFlagsBitMap(), enable, 16);
}

void configureAllIcmpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType /*asicType*/) {
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->proto(), enable, 58); // Icmp v6
    configureQualifier(acl->icmpType(), enable, 1); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 4); // Port unreachable
  } else {
    configureQualifier(acl->proto(), enable, 1); // Icmp v4
    configureQualifier(acl->icmpType(), enable, 3); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 3); // Port unreachable
  }
}

} // unnamed namespace

namespace facebook::fboss {

class AgentHwAclQualifierTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (!FLAGS_enable_acl_table_group) {
      return {production_features::ProductionFeature::SINGLE_ACL_TABLE};
    }
    return {production_features::ProductionFeature::MULTI_ACL_TABLE};
  }

  void configureAllHwQualifiers(
      cfg::AclEntry* acl,
      bool enable,
      SwitchID switchID = SwitchID(0)) {
    auto masterLogicalPorts =
        getAgentEnsemble()->masterLogicalInterfacePortIds({switchID});
    configureQualifier(acl->srcPort(), enable, masterLogicalPorts[0]);
    if ((hwAsicForSwitch(switchID)->getAsicType() !=
         cfg::AsicType::ASIC_TYPE_JERICHO2) &&
        (hwAsicForSwitch(switchID)->getAsicType() !=
         cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // No out port support on J2. Out port not used in prod
      configureQualifier(acl->dstPort(), enable, masterLogicalPorts[1]);
    }
  }

  void configureAllL2QualifiersHelper(
      cfg::AclEntry* acl,
      SwitchID switchID = SwitchID(0)) {
    configureQualifier(acl->dstMac(), true, "00:11:22:33:44:55");
    /*
     * lookupClassL2 is not configured for Trident2 or else we run out of
     * resources.
     * Note: lookupclassL2 is needed for MH-NIC queue-per-host solution.
     * However, the solution is not applicable for Trident2 as we don't
     * implement queues on trident2.
     */
    auto asic = hwAsicForSwitch(switchID);

    auto switchType = getAgentEnsemble()
                          ->getSw()
                          ->getSwitchInfoTable()
                          .getSwitchIdToSwitchInfo()
                          .at(switchID)
                          .switchType()
                          .value();
    if (asic->getAsicType() != cfg::AsicType::ASIC_TYPE_TRIDENT2 &&
        // L2 switching only on NPU type switch
        switchType == cfg::SwitchType::NPU) {
      configureQualifier(
          acl->lookupClassL2(),
          true,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
    }
  }

  cfg::AsicType getAsicType(SwitchID switchID = SwitchID(0)) {
    return hwAsicForSwitch(switchID)->getAsicType();
  }
};

TEST_F(AgentHwAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    auto* acl1 = utility::addAcl(&newCfg, "ip4_tcp", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP4, getAsicType());
    configureAllTcpQualifiers(acl1, true, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "ip4_tcp", FLAGS_enable_acl_table_group)
            ->toThrift();
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIp6TcpQualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    auto* acl1 = utility::addAcl(&newCfg, "ip6_tcp", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP6, this->getAsicType());
    configureAllTcpQualifiers(acl1, true, getAsicType());

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "ip6_tcp", FLAGS_enable_acl_table_group)
            ->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIcmp4Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    auto* acl1 = utility::addAcl(&newCfg, "icmp4", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP4, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "icmp4", FLAGS_enable_acl_table_group)
            ->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIcmp6Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    auto* acl1 = utility::addAcl(&newCfg, "icmp6", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP6, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "icmp6", FLAGS_enable_acl_table_group)
            ->toThrift();
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
