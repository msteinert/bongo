// Copyright Exegy, Inc.

#pragma once

#include <iostream>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <bongo/chan.h>

namespace bongo::time {

template <
  typename Rep,
  typename Period = std::ratio<1>,
  typename Clock = std::chrono::system_clock
>
class timer {
 public:
  using chan_type = chan<typename Clock::duration>;
  using recv_type = typename chan_type::recv_type;

 private:
  chan_type chan_ = chan_type{1};
  std::mutex mutex_;
  std::condition_variable cond_;
  bool active_ = false;
  std::thread thread_;

 public:
  timer() = default;
  timer(std::chrono::duration<Rep, Period> d)
      : active_{true}
      , thread_{&timer::start, this, std::move(d)} {}
  timer(std::chrono::duration<Rep, Period> d, std::function<void()> f)
      : active_{true}
      , thread_{&timer::start0, this, std::move(d), std::move(f)} {}

  ~timer() { stop(); }
  chan_type& c() { return chan_; }

  void reset(std::chrono::duration<Rep, Period> d) {
    std::unique_lock lock{mutex_};
    reset_locked();
    thread_ = std::thread{&timer::start, this, std::move(d)};
  }

  void reset(std::chrono::duration<Rep, Period> d, std::function<void()> f) {
    std::unique_lock lock{mutex_};
    reset_locked();
    thread_ = std::thread{&timer::start0, this, std::move(d), std::move(f)};
  }

  bool stop() {
    std::unique_lock lock{mutex_};
    auto active = active_;
    if (active) {
      active_ = false;
      lock.unlock();
      cond_.notify_one();
    }
    if (thread_.joinable()) {
      thread_.join();
    }
    return active;
  }

 private:
  void reset_locked() {
    if (active_) {
      throw logic_error{"reset on active timer"};
    }
    if (thread_.joinable()) {
      thread_.join();
    }
    active_ = true;
  }

  bool stopped(std::chrono::duration<Rep, Period> const& d) {
    std::unique_lock lock{mutex_};
    std::cv_status status = std::cv_status::no_timeout;
    while (active_) {
      status = cond_.wait_for(lock, d);
      if (status == std::cv_status::timeout) {
        break;
      }
    }
    active_ = false;
    return status == std::cv_status::no_timeout ? true : false;
  }

  void start(std::chrono::duration<Rep, Period>&& d) {
    auto begin = Clock::now();
    if (!stopped(d)) {
      chan_ << Clock::now() - begin;
    }
  }

  void start0(std::chrono::duration<Rep, Period>&& d, std::function<void()>&& f) {
    if (!stopped(d)) {
      f();
    }
  }
};

}  // namespace bongo::time
