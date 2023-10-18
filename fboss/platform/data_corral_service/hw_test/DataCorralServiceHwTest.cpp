/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/Flags.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/DataCorralServiceThrift.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/data_corral_service_types.h"

using namespace facebook::fboss::platform::data_corral_service;

namespace {
class DataCorralServiceHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    thriftHandler_ = std::make_shared<DataCorralServiceThriftHandler>();
  }

  void TearDown() override {}

 protected:
  DataCorralFruidReadResponse getFruid(bool uncached) {
    auto client = apache::thrift::makeTestClient(thriftHandler_);
    DataCorralFruidReadResponse resp;
    client->sync_getFruid(resp, uncached);
    return resp;
  }
  std::shared_ptr<DataCorralServiceThriftHandler> thriftHandler_;
};
} // namespace

TEST_F(DataCorralServiceHwTest, getCachedFruid) {
  EXPECT_GT(getFruid(false).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceHwTest, getUncachedFruid) {
  EXPECT_GT(getFruid(true).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceHwTest, testThrift) {
  apache::thrift::ScopedServerInterfaceThread server(thriftHandler_);
  auto client =
      server.newClient<apache::thrift::Client<DataCorralServiceThrift>>();
  DataCorralFruidReadResponse response;
  client->sync_getFruid(response, false);
  EXPECT_GT(response.fruidData()->size(), 0);
}
