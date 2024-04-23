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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

namespace facebook::fboss {

class RouteUpdateWrapper;

namespace utility {

void programRoutes(
    RouteUpdateWrapper& routeUpdater,
    RouterID rid,
    ClientID client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks);

void unprogramRoutes(
    RouteUpdateWrapper& routeUpdater,
    RouterID rid,
    ClientID client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks);

} // namespace utility
} // namespace facebook::fboss
