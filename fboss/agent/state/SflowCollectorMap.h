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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using SflowCollectorMapTraits = NodeMapTraits<std::string, SflowCollector>;

struct SflowCollectorMapThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::SflowCollectorFields> {
  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }
};

/*
 * A container for the set of collectors.
 */
class SflowCollectorMap : public ThriftyNodeMapT<
                              SflowCollectorMap,
                              SflowCollectorMapTraits,
                              SflowCollectorMapThriftTraits> {
 public:
  SflowCollectorMap() = default;
  ~SflowCollectorMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
