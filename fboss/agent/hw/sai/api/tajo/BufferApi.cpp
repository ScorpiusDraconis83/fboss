// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/BufferApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiBufferProfileTraits::Attributes::AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
