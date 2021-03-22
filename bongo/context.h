// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <any>
#include <exception>
#include <functional>
#include <chrono>
#include <functional>
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

using cancel_func = std::function<void()>;

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

class cancel_context : public context {
  std::shared_ptr<context> parent_;
  std::optional<std::chrono::system_clock::time_point> deadline_ = std::nullopt;

  std::mutex mutex_;
  chan<std::monostate> done_;
  std::unordered_set<context*> children_;
  std::exception_ptr err_;

 public:
  cancel_context(std::shared_ptr<context> parent);
  std::optional<std::chrono::system_clock::time_point> deadline() override;
  chan<std::monostate>* done() override;
  std::exception_ptr err() override;
  std::any value(std::string const& k) override;

  void cancel();
  void cancel(bool remove, std::exception_ptr err) override;
  void add(context* child) override;
  void remove(context* child) override;
  void deadline(std::chrono::system_clock::time_point tp);
};

class value_context : public context {
  std::shared_ptr<context> parent_;
  std::string key_;
  std::any value_;

 public:
  value_context(std::shared_ptr<context> parent, std::string key, std::any value)
      : parent_{std::move(parent)}
      , key_{std::move(key)}
      , value_{std::move(value)} {}

  std::optional<std::chrono::system_clock::time_point> deadline() override { return parent_->deadline(); }
  chan<std::monostate>* done() override { return parent_->done(); }
  std::exception_ptr err() override { return parent_->err(); }
  std::any value(std::string const& k) override {
    return k == key_ ? value_ : parent_->value(k);
  }

  void cancel(bool remove, std::exception_ptr err) override { parent_->cancel(remove, err); }
  void add(context* child) override { parent_->add(child); }
  void remove(context* child) override { parent_->remove(child); }
};

// Factory functions
std::shared_ptr<context> background();
std::shared_ptr<context> todo();

std::pair<std::shared_ptr<context>, cancel_func>
with_cancel(std::shared_ptr<context> parent);

template <typename Rep, typename Period = std::ratio<1>>
std::pair<std::shared_ptr<context>, cancel_func>
with_timeout(std::shared_ptr<context> parent, std::chrono::duration<Rep, Period> dur) {
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  ctx->deadline(std::chrono::system_clock::now() + dur);
  auto timer = std::make_shared<bongo::time::timer<Rep, Period>>(
      dur, [ctx]() { ctx->cancel(true, deadline_exceeded); });
  return std::make_pair(ctx, [timer, ctx]() {
    timer->stop();
    ctx->cancel();
  });
}

template <typename Clock, typename Duration = typename Clock::duration>
std::pair<std::shared_ptr<context>, cancel_func>
with_deadline(std::shared_ptr<context> parent, std::chrono::time_point<Clock, Duration> tp) {
  auto cur = parent->deadline();
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  if (cur && *cur < tp) {
    // Current deadline is sooner than the new one
    return std::make_pair(ctx, [ctx]() { ctx->cancel(); });
  }
  ctx->deadline(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp));
  auto dur = tp - Clock::now();
  if (dur <= typename Clock::duration{0}) {
    // Deadline already passed
    ctx->cancel(true, deadline_exceeded);
    return std::make_pair(ctx, [ctx]() { ctx->cancel(false, canceled); });
  }
  auto timer = std::make_shared<bongo::time::timer<typename Clock::rep, typename Clock::period>>(
      dur, [ctx]() { ctx->cancel(true, deadline_exceeded); });
  return std::make_pair(ctx, [timer, ctx]() {
    timer->stop();
    ctx->cancel();
  });
}

std::shared_ptr<context>
with_value(std::shared_ptr<context> parent, std::string value, std::any key);

}  // namespace bongo::context
