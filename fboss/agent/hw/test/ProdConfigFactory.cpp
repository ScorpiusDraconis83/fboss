/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ProdConfigFactory.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/platforms/common/PlatformMode.h"

namespace facebook::fboss::utility {

/*
 * Setup and enable Olympic QoS on the given config. Common among all platforms;
 * most use only Olympic QoS, but some roles such as MHNIC RSWs use this
 * together with queue-per-host mapping.
 */
void addOlympicQosToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  addOlympicQosMaps(config);
  auto streamType = *hwAsic->getQueueStreamTypes(false).begin();
  addOlympicQueueConfig(&config, streamType, hwAsic);
}

/*
 * Enable load balancing on provided config. Uses an enum, declared in
 * ConfigFactory.h, to decide whether to apply full-hash or half-hash config.
 * These are the only two current hashing algorithms at the time of writing; if
 * any more are added, this function and the enum can be modified accordingly.
 */
void addLoadBalancerToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    LBHash hashType) {
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();
  if (!hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    return;
  }
  switch (hashType) {
    case LBHash::FULL_HASH:
      config.loadBalancers_ref()->push_back(getEcmpFullHashConfig(platform));
      break;
    case LBHash::HALF_HASH:
      config.loadBalancers_ref()->push_back(getEcmpHalfHashConfig(platform));
      break;
    default:
      throw FbossError("invalid hashing option ", hashType);
      break;
  }
}

/*
 * Straightforward convenience function. Uses values defined in getUplinksCount,
 * from HwInitAndExitBenchmarkHelper.cpp, to return the number of uplinks for
 * each platform.
 */
uint16_t uplinksCountFromSwitch(const HwSwitch* hwSwitch) {
  auto mode = hwSwitch->getPlatform()->getMode();
  using PM = PlatformMode;
  switch (mode) {
    case PM::WEDGE:
    case PM::WEDGE100:
    case PM::WEDGE400C:
    case PM::WEDGE400:
    case PM::YAMP:
    case PM::MINIPACK:
    case PM::ELBERT:
    case PM::FUJI:
    case PM::CLOUDRIPPER:
    case PM::GALAXY_LC:
    case PM::GALAXY_FC:
      return 4;
    default:
      throw FbossError(
          "provided PlatformMode: ",
          mode,
          " has not been defined for uplinksCountFromSwitch");
      break;
  }
}

} // namespace facebook::fboss::utility
