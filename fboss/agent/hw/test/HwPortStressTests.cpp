/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include <string>

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace facebook::fboss {

class HwPortStressTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }
};

TEST_F(HwPortStressTest, adminStateToggle) {
  auto setup = [=, this]() { applyNewConfig(initialConfig()); };

  auto verify = [=, this]() {
    // Use 2nd port as on some platforms, first port is rcy port
    // which will never flap in practice
    auto portId = PortID(masterLogicalPortIds()[1]);
    for (auto i = 0; i < 500; ++i) {
      auto newState = getProgrammedState();
      auto port = newState->getPorts()->getNodeIf(portId);
      auto newPort = port->modify(&newState);
      auto newAdminState = newPort->isEnabled() ? cfg::PortState::DISABLED
                                                : cfg::PortState::ENABLED;
      newPort->setAdminState(newAdminState);
      applyNewState(newState);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortStressTest, linkStateToggle) {
  auto setup = [=, this]() { applyNewConfig(initialConfig()); };

  auto verify = [=, this]() {
    // Use 2nd port as on some platforms, first port is rcy port
    // which will never flap in practice
    auto portId = PortID(masterLogicalPortIds()[1]);
    for (auto i = 0; i < 500; ++i) {
      getHwSwitchEnsemble()->getLinkToggler()->bringDownPorts({portId});
      getHwSwitchEnsemble()->getLinkToggler()->bringUpPorts({portId});
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
