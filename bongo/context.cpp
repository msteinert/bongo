// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>

#include "bongo/context.h"

namespace bongo::context {

cancel_context::cancel_context(context_type parent)
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

std::optional<std::chrono::system_clock::time_point> value_context::deadline() {
  return parent_->deadline();
}

chan<std::monostate>* value_context::done() {
  return parent_->done();
}

std::exception_ptr value_context::err() {
  return parent_->err();
}

std::any value_context::value(std::string const& k) {
  return k == key_ ? value_ : parent_->value(k);
}

void value_context::cancel(bool remove, std::exception_ptr err) {
  parent_->cancel(remove, err);
}

void value_context::add(context* child) {
  parent_->add(child);
}

void value_context::remove(context* child) {
  parent_->remove(child);
}

context_type background() {
  static context_type ctx = std::make_shared<context>();
  return ctx;
}

context_type todo() {
  static context_type ctx = std::make_shared<context>();
  return ctx;
}

cancelable_context with_cancel(context_type parent) {
  auto ctx = std::make_shared<cancel_context>(std::move(parent));
  return std::make_pair(ctx, [ctx]() { ctx->cancel(); });
}

context_type with_value(context_type parent, std::string value, std::any key) {
  return std::make_shared<value_context>(std::move(parent), std::move(value), std::move(key));
}

}  // namespace bongo::context
