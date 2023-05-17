// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/facebook/gen-cpp2-thriftpath/fsdb_model.h" // @manual=//fboss/fsdb/if/facebook:fsdb_model-cpp2-thriftpath

#include <memory>

namespace facebook::fboss {
namespace fsdb {
class FsdbPubSubManager;
}

class LedManager;

/*
 * FsdbSwitchStateSubscriber class:
 *
 * This class subscribes to FSDB for the Port Info state path. The callback
 * function is called in FSDB callback thread and it will update the Port Info
 * map in Led Service.
 */
class FsdbSwitchStateSubscriber {
 public:
  explicit FsdbSwitchStateSubscriber(fsdb::FsdbPubSubManager* pubSubMgr)
      : fsdbPubSubMgr_(pubSubMgr) {}

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_;
  }

  void subscribeToSwitchState(
      folly::Synchronized<std::map<uint16_t, fboss::state::PortFields>>&
          storage,
      LedManager* ledManager);

  // FSDB path:
  //     FsdbOperStateRoot root()
  //     .. AgentData agent()
  //        .. switch_state.SwitchState() switchState()
  //           .. map<i16, PortFields> portMap()
  static std::vector<std::string> getSwitchStatePath() {
    thriftpath::RootThriftPath<fsdb::FsdbOperStateRoot> rootPath_;
    return rootPath_.agent().switchState().portMaps().tokens();
  }

 private:
  template <typename T>
  void subscribeToState(
      std::vector<std::string> path,
      folly::Synchronized<T>& storage,
      LedManager* ledManager);

  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
};

} // namespace facebook::fboss
