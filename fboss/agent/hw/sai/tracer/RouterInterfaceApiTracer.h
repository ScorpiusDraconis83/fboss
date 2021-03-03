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

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_router_interface_api_t* wrappedRouterInterfaceApi();

SET_ATTRIBUTE_FUNC_DECLARATION(RouterInterface);

} // namespace facebook::fboss
