// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/SubscriptionPathStore.h"
#include "fboss/fsdb/oper/SubscriptionManager.h"

namespace facebook::fboss::fsdb {

DEFINE_bool(lazyPathStoreCreation, true, "Lazy path store creation");

void SubscriptionPathStore::add(Subscription* subscription) {
  auto path = subscription->path();
  add(path.begin(), path.end(), subscription);
}

void SubscriptionPathStore::remove(Subscription* subscription) {
  auto path = subscription->path();
  remove(path.begin(), path.end(), subscription);
}

void SubscriptionPathStore::incrementallyResolve(
    SubscriptionManagerBase& manager,
    std::shared_ptr<ExtendedSubscription> subscription,
    std::size_t pathIdx,
    std::vector<std::string>& pathSoFar) {
  // This incrementally extends our subscription tree based on an extended
  // path. Logic is to build up tree until next wildcard, or end of the path.

  const auto& path = subscription->pathAt(pathIdx).path().value();
  auto idx = pathSoFar.size();

  if (idx == path.size()) {
    // have reached the end of the extended path, add a
    // resolved Subscription*
    XLOG(DBG2) << this << ": Saving fully resolved extended subscription at "
               << folly::join('/', pathSoFar);
    auto resolvedSub = subscription->resolve(pathSoFar);
    manager.registerSubscription(std::move(resolvedSub));
    return;
  } else if (path.at(idx).getType() != OperPathElem::Type::raw) {
    // we hit another wildcard element. Add a partially resolved subscription
    PartiallyResolvedExtendedSubscription partial;
    partial.subscription = subscription;
    partial.pathIdx = pathIdx;
    partial.elemIdx = idx;

    XLOG(DBG2) << this << ": Saving partially resolved subscription at "
               << folly::join('/', pathSoFar);
    partiallyResolvedSubs_.emplace_back(std::move(partial));

    return;
  }

  const auto& key = path.at(idx).raw_ref().value();
  if (auto it = children_.find(key); it == children_.end()) {
    children_.emplace(key, std::make_shared<SubscriptionPathStore>());
  }
  pathSoFar.emplace_back(key);
  children_.at(key)->incrementallyResolve(
      manager, std::move(subscription), pathIdx, pathSoFar);
  pathSoFar.pop_back();
}

void SubscriptionPathStore::processAddedPath(
    SubscriptionManagerBase& manager,
    PathIter begin,
    PathIter curr,
    PathIter end) {
  // this takes in a newly added path and then ensures that we
  // resolve any relevant partially resolved subscriptions

  if (curr == end) {
    return;
  }

  auto key = *curr++;

  // if child PathStore is not already created, don't create one unless
  // there are any partially resolved subscriptions that match this key
  bool childStorePresent{true};
  if (auto it = children_.find(key); it == children_.end()) {
    if (!FLAGS_lazyPathStoreCreation) {
      children_.emplace(key, std::make_shared<SubscriptionPathStore>());
    } else {
      childStorePresent = false;
    }
  }

  auto it = partiallyResolvedSubs_.begin();
  while (it != partiallyResolvedSubs_.end()) {
    auto& partial = *it;

    auto subscription = partial.subscription.lock();
    if (!subscription) {
      XLOG(DBG2) << this << ": Removing defunct partial subscription";
      it = partiallyResolvedSubs_.erase(it);
      continue;
    }

    if (auto findIt = partial.previouslyResolved.find(key);
        findIt != partial.previouslyResolved.end()) {
      ++it;
      continue;
    }
    partial.previouslyResolved.insert(key);

    const auto& path = subscription->pathAt(partial.pathIdx).path().value();
    const auto& elem = path.at(partial.elemIdx);
    bool matches{false};

    if (elem.getType() == OperPathElem::Type::any) {
      matches = true;
    } else if (auto regexStr = elem.regex_ref()) {
      matches = re2::RE2::FullMatch(key, *regexStr);
    }

    if (matches) {
      // we match this wildcard, incrementally resolve the
      // partial subscription to next wilcard or the end.
      std::vector<std::string> pathSoFar(begin, curr);
      if (!childStorePresent) {
        children_.emplace(key, std::make_shared<SubscriptionPathStore>());
        childStorePresent = true;
      }
      children_.at(key)->incrementallyResolve(
          manager, std::move(subscription), partial.pathIdx, pathSoFar);
    }

    ++it;
  }

  // if child PathStore is not created, that means there is no subscription
  // to resolve under this key. So process further only if child PathStore
  // is present.
  if (childStorePresent) {
    children_.at(key)->processAddedPath(manager, begin, curr, end);
  }
}

std::vector<Subscription*> SubscriptionPathStore::find(
    PathIter begin,
    PathIter end,
    std::unordered_set<LookupType> lookupTypes) {
  if (lookupTypes.empty()) {
    return {};
  }

  std::vector<Subscription*> gathered;

  if (begin == end) {
    if (lookupTypes.count(LookupType::TARGET)) {
      std::copy(
          subscriptions_.begin(),
          subscriptions_.end(),
          std::back_inserter(gathered));
    }

    if (lookupTypes.count(LookupType::CHILDREN)) {
      for (auto& [_name, child] : children_) {
        child->gatherChildren(gathered);
      }
    }
  } else {
    if (lookupTypes.count(LookupType::PARENTS)) {
      std::copy(
          subscriptions_.begin(),
          subscriptions_.end(),
          std::back_inserter(gathered));
    }

    auto key = *begin++;
    if (auto it = children_.find(key); it != children_.end()) {
      auto toAppend = it->second->find(begin, end, std::move(lookupTypes));
      std::copy(toAppend.begin(), toAppend.end(), std::back_inserter(gathered));
    }
  }

  return gathered;
}

SubscriptionPathStore* FOLLY_NULLABLE
SubscriptionPathStore::child(const std::string& key) const {
  if (auto it = children_.find(key); it != children_.end()) {
    return it->second.get();
  } else {
    return nullptr;
  }
}

SubscriptionPathStore* SubscriptionPathStore::getOrCreateChild(
    const std::string& key) {
  auto [it, added] =
      children_.emplace(key, std::make_shared<SubscriptionPathStore>());
  return it->second.get();
}

void SubscriptionPathStore::removeChild(const std::string& key) {
  if (auto it = children_.find(key); it == children_.end()) {
    return;
  }
  auto child = children_[key];
  CHECK_EQ(children_[key]->numSubsRecursive(), 0);
  children_.erase(key);
  return;
}

void SubscriptionPathStore::gatherChildren(
    std::vector<Subscription*>& gathered) {
  std::copy(
      subscriptions_.begin(),
      subscriptions_.end(),
      std::back_inserter(gathered));

  for (auto& [_name, child] : children_) {
    child->gatherChildren(gathered);
  }
}

void SubscriptionPathStore::add(
    PathIter begin,
    PathIter end,
    Subscription* subscription) {
  if (begin == end) {
    subscriptions_.push_back(subscription);
    incrementCounts(subscription, false);
    return;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it == children_.end()) {
    children_.emplace(key, std::make_shared<SubscriptionPathStore>());
  }

  children_[key]->add(begin, end, subscription);
  incrementCounts(subscription, true);
}

bool SubscriptionPathStore::remove(
    PathIter begin,
    PathIter end,
    Subscription* subscription) {
  if (begin == end) {
    auto it =
        std::find(subscriptions_.begin(), subscriptions_.end(), subscription);
    if (it != subscriptions_.end()) {
      subscriptions_.erase(it);
      decrementCounts(subscription, false);
      return true;
    }
    return false;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it == children_.end()) {
    return false;
  }

  if (children_[key]->remove(begin, end, subscription)) {
    decrementCounts(subscription, true);
    return true;
  }

  return false;
}

void SubscriptionPathStore::incrementCounts(
    Subscription* subscription,
    bool isChild) {
  if (isChild) {
    ++numChildSubs_;
  } else {
    ++numSubs_;
  }
}

void SubscriptionPathStore::decrementCounts(
    Subscription* subscription,
    bool isChild) {
  if (isChild) {
    --numChildSubs_;
  } else {
    --numSubs_;
  }
}

const SubscriptionPathStore* FOLLY_NULLABLE
SubscriptionPathStore::findStore(PathIter begin, PathIter end) const {
  if (begin == end) {
    return this;
  }

  auto key = *begin++;
  if (auto it = children_.find(key); it != children_.end()) {
    return it->second->findStore(begin, end);
  }

  return nullptr;
}

void SubscriptionPathStore::debugPrint() const {
  std::vector<std::string> emptyPathSoFar;
  debugPrint(emptyPathSoFar);
}

void SubscriptionPathStore::debugPrint(
    std::vector<std::string>& pathSoFar) const {
  XLOG(INFO);
  XLOG(INFO) << "SubscriptionPathStore at " << '/'
             << folly::join('/', pathSoFar);
  XLOG(INFO) << "children_.size()=" << children_.size();
  XLOG(INFO) << "partiallyResolvedSubs_.size()="
             << partiallyResolvedSubs_.size();
  if (!children_.empty()) {
    std::vector<std::string> children;
    for (const auto& [child, _] : children_) {
      children.emplace_back(child);
    }
    XLOG(INFO) << "Children: " << folly::join(',', children);
  }
  if (!partiallyResolvedSubs_.empty()) {
    XLOG(INFO) << "Partially Resolved: ";
    for (const auto& partial : partiallyResolvedSubs_) {
      partial.debugPrint();
    }
  }
  for (const auto& [child, childStore] : children_) {
    pathSoFar.emplace_back(child);
    childStore->debugPrint(pathSoFar);
    pathSoFar.pop_back();
  }
}

void PartiallyResolvedExtendedSubscription::debugPrint() const {
  XLOG(INFO) << "\tsubscription=" << subscription.lock().get()
             << ", pathIdx=" << pathIdx << ", elemIdx=" << elemIdx
             << ", previouslyResolved=" << folly::join(',', previouslyResolved);
}

} // namespace facebook::fboss::fsdb
