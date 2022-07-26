// Copyright The Go Authors.

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <bongo/bongo.h>
#include <bongo/time/time.h>

namespace bongo::time {

/**
 * A timer event.
 *
 * - https://golang.org/pkg/time/#Timer
 */
template <typename T = std::chrono::system_clock> requires Clock<T>
class timer {
 public:
  using chan_type = chan<typename T::duration>;
  using recv_type = typename chan_type::recv_type;

 private:
  chan_type chan_ = chan_type{1};
  chan<typename T::duration> wait_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool active_ = false;
  std::thread thread_;

 public:
  timer() = default;
  timer(typename T::duration d)
      : thread_{&timer::start, this} { wait_ << d; }
  template <typename Function>
  timer(typename T::duration d, Function&& fn)
      : thread_{&timer::start0<Function>, this, std::forward<Function>(fn)} { wait_ << d; }

  ~timer() try {
    stop();
    wait_.close();
    if (thread_.joinable()) {
      thread_.join();
    }
  } catch (...) {}

  timer(timer const& other) = delete;
  timer& operator=(timer const& other) = delete;
  timer(timer&& other) = delete;
  timer& operator=(timer&& other) = delete;

  chan_type& c() { return chan_; }

  bool reset(typename T::duration d) {
    std::unique_lock lock{mutex_};
    bool active = active_;
    lock.unlock();
    wait_ << d;
    return active;
  }

  bool stop() {
    std::unique_lock lock{mutex_};
    auto active = active_;
    if (active) {
      active_ = false;
      lock.unlock();
      cond_.notify_one();
    }
    return active;
  }

 private:
  void start() {
    typename decltype(wait_)::recv_type dur;
    for (;;) {
      // Wait for a new duration
      dur << wait_;
      if (!dur.has_value()) {
        return;
      }

      // Begin the timer
      std::unique_lock lock{mutex_};
      active_ = true;
      auto begin = T::now();
      cond_.wait_for(lock, *dur);

      // Check if the timer was canceled
      if (!active_) {
        continue;
      }

      // Timer fired
      active_ = false;
      lock.unlock();
      chan_ << T::now() - begin;
    }
  }

  template <typename Function>
  void start0(Function&& fn) {
    typename decltype(wait_)::recv_type dur;
    for (;;) {
      // Wait for a new duration
      dur << wait_;
      if (!dur.has_value()) {
        return;
      }

      // Begin the timer
      std::unique_lock lock{mutex_};
      active_ = true;
      cond_.wait_for(lock, *dur);

      // Check if the timer was canceled
      if (!active_) {
        continue;
      }

      // Timer fired
      active_ = false;
      lock.unlock();
      fn();
    }
  }
};

}  // namespace bongo::time
