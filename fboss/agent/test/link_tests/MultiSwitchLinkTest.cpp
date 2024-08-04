// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <gflags/gflags.h>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/MultiSwitchLinkTest.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types_custom_protocol.h"

DECLARE_bool(enable_macsec);

DECLARE_bool(skip_drain_check_for_prbs);

DEFINE_bool(
    link_stress_test,
    false,
    "enable to run stress tests (longer duration + more iterations)");

namespace {
const std::vector<std::string> kRestartQsfpService = {
    "/bin/systemctl",
    "restart",
    "qsfp_service_for_testing"};

const std::string kForceColdbootQsfpSvcFileName =
    "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";

// This file stores all non-validated transceiver configurations found while
// running getAllTransceiverConfigValidationStatuses() in EmptyLinkTest. Each
// configuration will be in JSON format. This file is then downloaded by the
// Netcastle test runner to parse it and log these configurations to Scuba.
// Matching file definition is located in
// fbcode/neteng/netcastle/teams/fboss/constants.py.
const std::string kTransceiverConfigJsonsForScuba =
    "/tmp/transceiver_config_jsons_for_scuba.log";
} // namespace

namespace facebook::fboss {

void MultiSwitchLinkTest::SetUp() {
  AgentEnsembleTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  waitForAllCabledPorts(true, 60, 5s);
  waitForAllTransceiverStates(true, 60, 5s);
  XLOG(DBG2) << "Multi Switch Link Test setup ready";
}

void MultiSwitchLinkTest::restartQsfpService(bool coldboot) const {
  if (coldboot) {
    createFile(kForceColdbootQsfpSvcFileName);
    XLOG(DBG2) << "Restarting QSFP Service in coldboot mode";
  } else {
    XLOG(DBG2) << "Restarting QSFP Service in warmboot mode";
  }

  folly::Subprocess(kRestartQsfpService).waitChecked();
}

void MultiSwitchLinkTest::TearDown() {
  // Expect the qsfp service to be running at the end of the tests
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  EXPECT_EQ(
      facebook::fb303::cpp2::fb_status::ALIVE,
      qsfpServiceClient.get()->sync_getStatus())
      << "QSFP Service no longer alive after the test";
  EXPECT_EQ(
      QsfpServiceRunState::ACTIVE,
      qsfpServiceClient.get()->sync_getQsfpServiceRunState())
      << "QSFP Service run state no longer active after the test";
  AgentEnsembleTest::TearDown();
}

void MultiSwitchLinkTest::setCmdLineFlagOverrides() const {
  FLAGS_enable_macsec = true;
  FLAGS_skip_drain_check_for_prbs = true;
  AgentEnsembleTest::setCmdLineFlagOverrides();
}

void MultiSwitchLinkTest::overrideL2LearningConfig(
    bool swLearning,
    int ageTimer) {
  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  cfg::AgentConfig testConfig = agentConfig->thrift;
  if (swLearning) {
    testConfig.sw()->switchSettings()->l2LearningMode() =
        cfg::L2LearningMode::SOFTWARE;
  } else {
    testConfig.sw()->switchSettings()->l2LearningMode() =
        cfg::L2LearningMode::HARDWARE;
  }
  testConfig.sw()->switchSettings()->l2AgeTimerSeconds() = ageTimer;
  auto newAgentConfig = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  newAgentConfig.dumpConfig(getTestConfigPath());
  FLAGS_config = getTestConfigPath();
  reloadPlatformConfig();
}

// Waits till the link status of the ports in cabledPorts vector reaches
// the expected state
void MultiSwitchLinkTest::waitForAllCabledPorts(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForLinkStatus(getCabledPorts(), up, retries, msBetweenRetry);
}

void MultiSwitchLinkTest::waitForAllTransceiverStates(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForStateMachineState(
      cabledTransceivers_,
      up ? TransceiverStateMachineState::ACTIVE
         : TransceiverStateMachineState::INACTIVE,
      retries,
      msBetweenRetry);
}

// Initializes the vector that holds the ports that are expected to be cabled.
// If the expectedLLDPValues in the switch config has an entry, we expect
// that port to take part in the test
void MultiSwitchLinkTest::initializeCabledPorts() {
  const auto& platformPorts = getSw()->getPlatformMapping()->getPlatformPorts();

  auto swConfig = getSw()->getConfig();
  const auto& chips = getSw()->getPlatformMapping()->getChips();
  for (const auto& port : *swConfig.ports()) {
    if (!(*port.expectedLLDPValues()).empty()) {
      auto portID = *port.logicalID();
      cabledPorts_.push_back(PortID(portID));
      if (*port.portType() == cfg::PortType::FABRIC_PORT) {
        cabledFabricPorts_.push_back(PortID(portID));
      }
      const auto platformPortEntry = platformPorts.find(portID);
      EXPECT_TRUE(platformPortEntry != platformPorts.end())
          << "Can't find port:" << portID << " in PlatformMapping";
      auto transceiverID =
          utility::getTransceiverId(platformPortEntry->second, chips);
      if (transceiverID.has_value()) {
        cabledTransceivers_.insert(*transceiverID);
      }
    }
  }
}

const std::vector<PortID>& MultiSwitchLinkTest::getCabledPorts() const {
  return cabledPorts_;
}

boost::container::flat_set<PortDescriptor>
MultiSwitchLinkTest::getVlanOwningCabledPorts() const {
  boost::container::flat_set<PortDescriptor> ecmpPorts;
  auto vlanOwningPorts =
      utility::getPortsWithExclusiveVlanMembership(getSw()->getState());
  for (auto port : getCabledPorts()) {
    if (vlanOwningPorts.find(PortDescriptor(port)) != vlanOwningPorts.end()) {
      ecmpPorts.insert(PortDescriptor(port));
    }
  }
  return ecmpPorts;
}

std::string MultiSwitchLinkTest::getPortName(PortID portId) const {
  for (auto portMap : std::as_const(*getSw()->getState()->getPorts())) {
    for (auto port : std::as_const(*portMap.second)) {
      if (port.second->getID() == portId) {
        return port.second->getName();
      }
    }
  }
  throw FbossError("No port with ID: ", portId);
}

std::vector<std::string> MultiSwitchLinkTest::getPortName(
    const std::vector<PortID>& portIDs) const {
  std::vector<std::string> portNames;
  for (auto port : portIDs) {
    portNames.push_back(getPortName(port));
  }
  return portNames;
}

int multiSwitchLinkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentEnsembleTest(argc, argv, initPlatformFn, streamType);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
