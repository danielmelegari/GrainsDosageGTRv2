#pragma once
#include "trial_license_win.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace grains {
// Storage and DPAPI run only at construction and on the maintenance thread.
class TrialGuard {
  mutable std::atomic<bool> blocked_{true};
  std::chrono::steady_clock::time_point deadline_{};
  trial::Tick end_ = 0;
  std::mutex mutex_;
  std::condition_variable wake_;
  bool stopping_ = false;
  std::thread worker_;
public:
  TrialGuard() noexcept {
    try {
      trial::License license;
      if (!trial::loadLicense(license)) return;
      auto store = trial::Store::production();
      trial::State state{license.start, license.start};
      if (!store.refresh(trial::wallTime(), state, true)) return;
      const auto remaining = trial::duration - (state.last - state.start);
      deadline_ = std::chrono::steady_clock::now() + std::chrono::nanoseconds(remaining * 100);
      end_ = state.start + trial::duration;
      blocked_.store(false, std::memory_order_relaxed);
      worker_ = std::thread([this, store, state]() mutable {
        try {
          unsigned unavailableChecks = 0;
          std::unique_lock<std::mutex> lock(mutex_);
          while (!wake_.wait_for(lock, std::chrono::seconds(30), [this] { return stopping_; })) {
            lock.unlock();
            // The fixed signed expiry and monotonic deadline cannot be extended
            // by editing/deleting local state while this instance is running.
            const auto left = deadline_ - std::chrono::steady_clock::now();
            const auto secondsLeft = std::chrono::duration_cast<std::chrono::seconds>(left).count();
            const auto minimumTime = secondsLeft > 0 ? end_ - trial::Tick(secondsLeft) * trial::second : end_;
            // The verified signed license is immutable in memory. Reopening
            // its file and hardware registry every 30 seconds added failure
            // points without protecting this already-fixed expiration.
            const auto result = store.check(trial::wallTime(), state, false, minimumTime);
            if(result==trial::Refresh::unavailable)++unavailableChecks;else unavailableChecks=0;
            if (secondsLeft <= 0 || result==trial::Refresh::expired || result==trial::Refresh::invalid || unavailableChecks>=3) {
              blocked_.store(true, std::memory_order_relaxed);
            } else {
              // Recover from a temporary file/DPAPI failure. The signed end
              // and monotonic deadline remain fixed for this instance.
              blocked_.store(false, std::memory_order_relaxed);
            }
            lock.lock();
          }
        } catch (...) { blocked_.store(true, std::memory_order_relaxed); }
      });
    } catch (...) { blocked_.store(true, std::memory_order_relaxed); }
  }
  ~TrialGuard() {
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
    wake_.notify_one();
    if (worker_.joinable()) worker_.join();
  }
  bool expired() const {
    if (blocked_.load(std::memory_order_relaxed)) return true;
    if (std::chrono::steady_clock::now() >= deadline_ || trial::wallTime() >= end_) {
      blocked_.store(true, std::memory_order_relaxed);
      return true;
    }
    return false;
  }
};
} // namespace grains
