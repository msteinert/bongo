// Copyright The Go Authors.

#pragma once

#include <any>
#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <variant>

#include <bongo/bongo.h>
#include <bongo/context/error.h>
#include <bongo/time.h>

namespace bongo::context {

using cancel_func = std::function<void()>;

/**
 * The context type is a port of Go contexts.
 *
 * - https://golang.org/pkg/context/#Context
 */
struct context {
  virtual ~context() {}

  // Public API
  virtual std::optional<std::chrono::system_clock::time_point> deadline() { return std::nullopt; }
  virtual chan<std::monostate>* done() { return nullptr; }
  virtual std::error_code err() { return nil; }
  virtual std::any value(std::string_view) { return std::any{}; }

  // Implementation API
  virtual void cancel(bool, std::error_code) {}
  virtual void add(context*) {}
  virtual void remove(context*) {}
};

using context_type = std::shared_ptr<context>;
using cancelable_context = std::pair<context_type, cancel_func>;

class cancel_context : public context {
  context_type parent_;
  std::optional<std::chrono::system_clock::time_point> deadline_ = std::nullopt;

  std::mutex mutex_;
  chan<std::monostate> done_;
  std::unordered_set<context*> children_;
  std::error_code err_ = nil;

 public:
  cancel_context(context_type parent);
  std::optional<std::chrono::system_clock::time_point> deadline() override;
  chan<std::monostate>* done() override;
  std::error_code err() override;
  std::any value(std::string_view) override;

  void cancel();
  void cancel(bool remove, std::error_code err) override;
  void add(context* child) override;
  void remove(context* child) override;
  void deadline(std::chrono::system_clock::time_point tp);
};

class value_context : public context {
  context_type parent_;
  std::string key_;
  std::any value_;

 public:
  value_context(context_type parent, std::string key, std::any value)
      : parent_{std::move(parent)}
      , key_{std::move(key)}
      , value_{std::move(value)} {}

  std::optional<std::chrono::system_clock::time_point> deadline() override;
  chan<std::monostate>* done() override;
  std::error_code err() override;
  std::any value(std::string_view) override;

  void cancel(bool remove, std::error_code err) override;
  void add(context* child) override;
  void remove(context* child) override;
};

/**
 * Background is an empty top-level context.
 *
 * - https://golang.org/pkg/context/#Background
 */
context_type background();

/**
 * TODO is an empty top-level context for use as a placeholder.
 *
 * - https://golang.org/pkg/context/#TODO
 */
context_type todo();

/**
 * Cancel contexts are used to cancel a thread.
 *
 * - https://golang.org/pkg/context/#WithCancel
 */
cancelable_context with_cancel(context_type parent);

/**
 * Value contexts associate a key and value.
 *
 * - https://golang.org/pkg/context/#WithValue
 */
context_type with_value(context_type parent, std::string value, std::any key);

/**
 * Deadline contexts automatically cancel after the specified duration.
 *
 * - https://golang.org/pkg/context/#WithTimeout
 */
template <typename T = std::chrono::system_clock> requires time::Clock<T>
cancelable_context with_timeout(context_type parent, typename T::duration dur) {
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  ctx->deadline(std::chrono::system_clock::now() + dur);
  auto timer = std::make_shared<bongo::time::timer<T>>(
      dur, [ctx]() { ctx->cancel(true, error::deadline_exceeded); });
  return std::make_pair(ctx, [timer, ctx]() {
    timer->stop();
    ctx->cancel();
  });
}

/**
 * Timeout contexts automatically cancel at the specified time point.
 *
 * - https://golang.org/pkg/context/#WithDeadline
 */
template <typename T = std::chrono::system_clock> requires time::Clock<T>
cancelable_context with_deadline(context_type parent, typename T::time_point tp) {
  auto cur = parent->deadline();
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  if (cur && *cur < tp) {
    // Current deadline is sooner than the new one
    return std::make_pair(ctx, [ctx]() { ctx->cancel(); });
  }
  ctx->deadline(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp));
  auto dur = tp - T::now();
  if (dur <= typename T::duration{0}) {
    // Deadline already passed
    ctx->cancel(true, error::deadline_exceeded);
    return std::make_pair(ctx, [ctx]() { ctx->cancel(false, error::canceled); });
  }
  auto timer = std::make_shared<bongo::time::timer<T>>(
      dur, [ctx]() {
        ctx->cancel(true, error::deadline_exceeded);
      });
  return std::make_pair(ctx, [timer, ctx]() {
    timer->stop();
    ctx->cancel();
  });
}

}  // namespace bongo::context
