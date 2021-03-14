// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <any>
#include <exception>
#include <functional>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <unordered_set>
#include <utility>

#include <bongo/chan.h>
#include <bongo/time.h>

namespace bongo::context {

struct error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

inline std::exception_ptr canceled = std::make_exception_ptr(error{"context canceled"});
inline std::exception_ptr deadline_exceeded = std::make_exception_ptr(error{"context deadline exceeded"});

struct context {
  virtual ~context() {}
  virtual std::optional<std::chrono::system_clock::time_point> deadline() { return std::nullopt; }
  virtual chan<std::monostate>* done() { return nullptr; }
  virtual std::exception_ptr err() { return nullptr; }
  virtual std::any value(std::string const&) { return std::any{}; }

  virtual void cancel(bool, std::exception_ptr) {}
  virtual void add(context*) {}
  virtual void remove(context*) {}
};

context& background();
context& todo();

class cancel_context : public context {
  context& parent_;

  std::mutex mutex_;
  chan<std::monostate> done_;
  std::unordered_set<context*> children_;
  std::exception_ptr err_;

 public:
  cancel_context(context& parent);
  chan<std::monostate>* done() override { return &done_; }
  std::exception_ptr err() override;
  std::any value(std::string const& k) override { return parent_.value(k); }
  void cancel();

  void cancel(bool remove, std::exception_ptr err) override;
  void add(context* child) override;
  void remove(context* child) override;
};

template <typename Rep, typename Period = std::ratio<1>, typename Clock = std::chrono::system_clock>
class timer_context : public cancel_context {
  std::chrono::system_clock::time_point deadline_;
  bongo::time::timer<Rep, Period, Clock> timer_;

 public:
  timer_context(context& parent, std::chrono::duration<Rep, Period> d)
      : cancel_context{parent}
      , deadline_{std::chrono::system_clock::now() + d}
      , timer_{d, [this]() { timeout(); }} {}

  template <typename ClockType>
  timer_context(context& parent, std::chrono::time_point<ClockType, typename ClockType::duration> tp)
      : cancel_context{parent}
      , deadline_{std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp)} {
    auto cur = parent.deadline();
    if (cur && *cur < deadline_) {
      return;  // Current deadline is sooner than the new one
    }
    auto dur = deadline_ - ClockType::now();
    if (dur <= typename ClockType::duration{0}) {
      cancel_context::cancel(true, deadline_exceeded);  // Deadline already passed
      return;
    }
    timer_.reset(dur, [this]() { timeout(); });
  }

  std::optional<std::chrono::system_clock::time_point> deadline() override {
    return deadline_;
  }

  using cancel_context::value;
  using cancel_context::cancel;

  void cancel(bool remove, std::exception_ptr err) override {
    timer_.stop();
    cancel_context::cancel(remove, err);
  }

 private:
  void timeout() {
    cancel_context::cancel(true, deadline_exceeded);
  }
};

template <typename ClockType>
timer_context(context& parent, std::chrono::time_point<ClockType, typename ClockType::duration> tp)
  -> timer_context<typename ClockType::time_point::rep, typename ClockType::time_point::period, ClockType>;

class value_context : public context {
  context& parent_;
  std::string key_;
  std::any value_;

 public:
  value_context(context& parent, std::string key, std::any value)
      : parent_{parent}
      , key_{std::move(key)}
      , value_{std::move(value)} {}

  std::optional<std::chrono::system_clock::time_point> deadline() override { return parent_.deadline(); }
  chan<std::monostate>* done() override { return parent_.done(); }
  std::exception_ptr err() override { return parent_.err(); }
  std::any value(std::string const& k) override {
    return k == key_ ? value_ : parent_.value(k);
  }

  void cancel(bool remove, std::exception_ptr err) override { parent_.cancel(remove, err); }
  void add(context* child) override { parent_.add(child); }
  void remove(context* child) override { parent_.remove(child); }
};

// Factory functions

cancel_context with_cancel(context& parent);

template <typename Rep, typename Period = std::ratio<1>>
timer_context<Rep, Period> with_timeout(context& parent, std::chrono::duration<Rep, Period> d) {
  return timer_context{parent, d};
}

template <typename Clock, typename Duration = typename Clock::duration>
timer_context <typename Clock::time_point::rep, typename Clock::time_point::period, Clock>
with_deadline(context& parent, std::chrono::time_point<Clock, Duration> tp) {
  return timer_context{parent, tp};
}

value_context with_value(context& parent, std::string value, std::any key);

}  // namespace bongo::context
