// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>

#include "bongo/context.h"

namespace bongo::context {

cancel_context::cancel_context(std::shared_ptr<context> parent)
    : context{}
    , parent_{std::move(parent)} {
  parent_->add(this);
}

std::optional<std::chrono::system_clock::time_point> cancel_context::deadline() {
  return deadline_ ? deadline_ : parent_->deadline();
}

chan<std::monostate>* cancel_context::done() {
  return &done_;
}

std::exception_ptr cancel_context::err() {
  std::lock_guard lock{mutex_};
  return err_;
}

std::any cancel_context::value(std::string const& k) {
  return parent_->value(k);
}

void cancel_context::cancel() {
  cancel(true, canceled);
}

void cancel_context::cancel(bool remove, std::exception_ptr err) {
  std::unique_lock lock{mutex_};
  if (err_ == nullptr) {
    err_ = err;
    for (auto* child : children_) {
      child->cancel(false, err);
    }
    children_.clear();
    done_.close();
    lock.unlock();
    if (remove) {
      parent_->remove(this);
    }
  }
}

void cancel_context::add(context* child) {
  std::lock_guard lock{mutex_};
  children_.insert(child);
}

void cancel_context::remove(context* child) {
  std::lock_guard lock{mutex_};
  children_.erase(child);
}

void cancel_context::deadline(std::chrono::system_clock::time_point tp) {
  deadline_ = std::move(tp);
}

std::shared_ptr<context> background() {
  static std::shared_ptr<context> ctx = std::make_shared<context>();
  return ctx;
}

std::shared_ptr<context> todo() {
  static std::shared_ptr<context> ctx = std::make_shared<context>();
  return ctx;
}

std::pair<std::shared_ptr<context>, cancel_func>
with_cancel(std::shared_ptr<context> parent) {
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  return std::make_pair(ctx, [ctx]() { ctx->cancel(); });
}

std::shared_ptr<context>
with_value(std::shared_ptr<context> parent, std::string value, std::any key) {
  return std::make_shared<value_context>(std::move(parent), std::move(value), std::move(key));
}

}  // namespace bongo::context
