// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/NaivePeriodicSubscribableStorageBase.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/system/ThreadName.h>

#ifndef IS_OSS
#include "common/base/Proc.h"
namespace {
inline int64_t getMemoryUsage() {
  return facebook::Proc::getMemoryUsage();
}
} // namespace
#else
namespace {
inline int64_t getMemoryUsage() {
  return 0;
}
} // namespace
#endif

DEFINE_int32(
    storage_thread_heartbeat_ms,
    10000,
    "subscribable storage thread's heartbeat interval (ms)");

namespace facebook::fboss::fsdb {

NaivePeriodicSubscribableStorageBase::NaivePeriodicSubscribableStorageBase(
    std::chrono::milliseconds subscriptionServeInterval,
    std::chrono::milliseconds subscriptionHeartbeatInterval,
    bool trackMetadata,
    const std::string& metricPrefix,
    bool convertToIDPaths)
    : subscriptionServeInterval_(subscriptionServeInterval),
      subscriptionHeartbeatInterval_(subscriptionHeartbeatInterval),
      trackMetadata_(trackMetadata),
      rss_(fmt::format("{}.{}", metricPrefix, kRss)),
      serveSubMs_(fmt::format("{}.{}", metricPrefix, kServeSubMs)),
      serveSubNum_(fmt::format("{}.{}", metricPrefix, kServeSubNum)),
      convertSubsToIDPaths_(convertToIDPaths) {
  if (trackMetadata) {
    metadataTracker_ = std::make_unique<FsdbOperTreeMetadataTracker>();
  }

  // init metrics

  fb303::ThreadCachedServiceData::get()->addStatExportType(rss_, fb303::AVG);

  // elapsed time to serve each subscription
  // histogram range [0, 1s], 10ms width (100 bins)
  fb303::ThreadCachedServiceData::get()->addHistogram(serveSubMs_, 10, 0, 1000);
  fb303::ThreadCachedServiceData::get()->exportHistogram(
      serveSubMs_, 50, 95, 99);

  fb303::ThreadCachedServiceData::get()->addStatExportType(
      serveSubNum_, fb303::SUM);
}

FsdbOperTreeMetadataTracker NaivePeriodicSubscribableStorageBase::getMetadata()
    const {
  return metadataTracker_.withRLock([&](auto& tracker) {
    CHECK(tracker);
    return *tracker;
  });
}

void NaivePeriodicSubscribableStorageBase::start_impl() {
  auto runningLocked = running_.wlock();

  if (*runningLocked) {
    return;
  }

  subscriptionServingThread_ = std::make_unique<std::thread>([=, this] {
    folly::setThreadName("ServeSubscriptions");
    evb_.loopForever();
  });

  XLOG(DBG1) << "Starting subscribable storage thread heartbeat";
  auto heartbeatStatsFunc = [](int /* delay */, int /* backLog */) {};
  threadHeartbeat_ = std::make_shared<ThreadHeartbeat>(
      &evb_,
      "ServeSubscriptions",
      FLAGS_storage_thread_heartbeat_ms,
      heartbeatStatsFunc);

  // serve first heartbeat 1 interval away
  lastHeartbeatTime = std::chrono::steady_clock::now();

  backgroundScope_.add(serveSubscriptions().scheduleOn(&evb_));

  *runningLocked = true;
}

void NaivePeriodicSubscribableStorageBase::stop_impl() {
  XLOG(DBG1) << "Stopping subscribable storage";
  bool wasRunning{false};
  {
    auto runningLocked = running_.wlock();
    wasRunning = *runningLocked;
    // Set running_ to false and give up lock. Else serveSubscriptions
    // thread could block waiting on acquiring running_.rlock() forever.
    // That causes subscriptionServingThread_->join to later deadlock
    *runningLocked = false;
  }

  if (wasRunning) {
    XLOG(DBG1) << "Cancelling background scope";
    folly::coro::blockingWait(backgroundScope_.cancelAndJoinAsync());
    XLOG(DBG1) << "Stopping eventbase";
    evb_.runImmediatelyOrRunInEventBaseThreadAndWait(
        [this] { evb_.terminateLoopSoon(); });
    subscriptionServingThread_->join();
  }

  if (threadHeartbeat_) {
    XLOG(DBG1) << "Stopping subscribable storage thread heartbeat";
    threadHeartbeat_.reset();
  }
  XLOG(INFO) << "Stopped subscribable storage";
}

SubscriptionMetadataServer
NaivePeriodicSubscribableStorageBase::getCurrentMetadataServer() {
  std::optional<FsdbOperTreeMetadataTracker::PublisherRoot2Metadata> metadata;
  metadataTracker_.withRLock([&metadata](auto& tracker) {
    if (tracker) {
      metadata = tracker->getAllMetadata();
    }
  });
  return SubscriptionMetadataServer(std::move(metadata));
}

void NaivePeriodicSubscribableStorageBase::exportServeMetrics(
    std::chrono::steady_clock::time_point serveStartTime) const {
  int64_t memUsage = getMemoryUsage(); // RSS
  fb303::ThreadCachedServiceData::get()->addStatValue(
      rss_, memUsage, fb303::AVG);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - serveStartTime);
  if (elapsed.count() > 0) {
    // ignore idle cycles, only count busy ones
    fb303::ThreadCachedServiceData::get()->addHistogramValue(
        serveSubMs_, elapsed.count());
  }
  fb303::ThreadCachedServiceData::get()->addStatValue(
      serveSubNum_, 1, fb303::SUM);
}

std::optional<std::string>
NaivePeriodicSubscribableStorageBase::getPublisherRoot(
    PathIter begin,
    PathIter end) const {
  auto path = convertPath(ConcretePath(begin, end));
  return trackMetadata_
      ? std::make_optional(
            OperPathToPublisherRoot().publisherRoot(path.begin(), path.end()))
      : std::nullopt;
}

std::optional<std::string>
NaivePeriodicSubscribableStorageBase::getPublisherRoot(
    const std::vector<ExtendedOperPath>& paths) const {
  auto convertedPaths = convertExtPaths(paths);
  return trackMetadata_
      ? std::make_optional(
            OperPathToPublisherRoot().publisherRoot(convertedPaths))
      : std::nullopt;
}

std::optional<std::string>
NaivePeriodicSubscribableStorageBase::getPublisherRoot(
    ExtPathIter begin,
    ExtPathIter end) const {
  auto path = convertPath(ExtPath(begin, end));
  return trackMetadata_
      ? std::make_optional(
            OperPathToPublisherRoot().publisherRoot(path.begin(), path.end()))
      : std::nullopt;
}

void NaivePeriodicSubscribableStorageBase::updateMetadata(
    PathIter begin,
    PathIter end,
    const OperMetadata& metadata) {
  metadataTracker_.withWLock(
      [&](auto& tracker) {
        if (tracker) {
          auto publisherRoot = getPublisherRoot(begin, end);
          CHECK(publisherRoot)
              << "Publisher root must be available before metadata can "
              << "be tracked ";
          tracker->updateMetadata(*publisherRoot, metadata);
        }
      });
}

std::vector<ExtendedOperPath>
NaivePeriodicSubscribableStorageBase::convertExtPaths(
    const std::vector<ExtendedOperPath>& paths) const {
  if (!convertSubsToIDPaths_) {
    return paths;
  }
  std::vector<ExtendedOperPath> convertedPaths;
  convertedPaths.reserve(paths.size());
  for (const auto& path : paths) {
    ExtendedOperPath p = path;
    p.path() = convertPath(*path.path());
    convertedPaths.emplace_back(std::move(p));
  }
  return convertedPaths;
}

} // namespace facebook::fboss::fsdb
