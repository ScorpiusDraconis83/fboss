#pragma once
#include <vector>
#include "fboss/agent/Main.h"
#include "fboss/agent/test/prod_agent_tests/ProdAgentTests.h"

#include "fboss/agent/SetupThrift.h"

class HwSwitch;

namespace facebook::fboss {

void verifyHwSwitchHandler();

class ProdInvariantTest : public ProdAgentTests {
 protected:
  virtual void SetUp() override;
  virtual void setupConfigFlag() override;
  virtual cfg::SwitchConfig initialConfig();
  void verifyAcl();
  void verifyCopp();
  void verifySafeDiagCommands();
  void verifyLoadBalancing();
  void verifyDscpToQueueMapping();
  void verifyQueuePerHostMapping(bool dscpMarkingTest);
  std::vector<PortDescriptor> ecmpPorts_{};
  bool checkBaseConfigPortsEmpty();
  cfg::SwitchConfig getConfigFromFlag();
  void verifyThriftHandler();
  void verifySwSwitchHandler();
  void set_mmu_lossless(bool mmu_lossless) {
    _mmu_lossless_mode = mmu_lossless;
  }
  bool is_mmu_lossless_mode() {
    return _mmu_lossless_mode;
  }

 protected:
  std::optional<bool> useProdConfig_ = std::nullopt;

 private:
  std::vector<PortID> getEcmpPortIds();
  void sendTraffic();
  PortID getDownlinkPort();
  void setupAgentTestEcmp(const std::vector<PortDescriptor>& ecmpPorts);
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
  bool _mmu_lossless_mode = false;
};

int ProdInvariantTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
