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
#include <memory>

#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/mnpu/SplitAgentThriftSyncerClient.h"

#include <folly/io/async/EventBase.h>
#include <string>

namespace facebook::fboss {

class HwSwitch;

class LinkActiveEventSyncer
    : public ThriftSinkClient<multiswitch::LinkActiveEvent> {
 public:
  using EventSink =
      ThriftSinkClient<multiswitch::LinkActiveEvent>::EventNotifierSinkClient;

  LinkActiveEventSyncer(
      uint16_t serverPort,
      SwitchID switchId,
      folly::EventBase* connRetryEvb);

  static EventSink initLinkActiveEventSink(
      SwitchID switchId,
      apache::thrift::Client<multiswitch::MultiSwitchCtrl>* client);
};
} // namespace facebook::fboss
