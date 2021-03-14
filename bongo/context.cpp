// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <exception>
#include <mutex>
#include <utility>
#include <variant>

#include "bongo/context.h"

namespace bongo::context {
namespace {

context background_ctx;
context todo_ctx;

}

context& background() {
  return background_ctx;
}

context& todo() {
  return todo_ctx;
}

cancel_context::cancel_context(context& parent)
    : context{}
    , parent_{parent} {
  parent_.add(this);
}

std::exception_ptr cancel_context::err() {
  std::lock_guard lock{mutex_};
  return err_;
}

void cancel_context::cancel() {
  cancel(true, canceled);
}

void cancel_context::cancel(bool remove, std::exception_ptr err) {
  if (remove) {
    parent_.remove(this);
  }
  std::lock_guard lock{mutex_};
  done_.close();
  err_ = err;
  for (auto* child : children_) {
    child->cancel(false, err);
  }
  children_.clear();
}

void cancel_context::add(context* child) {
  std::lock_guard lock{mutex_};
  children_.insert(child);
}

void cancel_context::remove(context* child) {
  std::lock_guard lock{mutex_};
  children_.erase(child);
}

cancel_context with_cancel(context& parent) {
  return cancel_context{parent};
}

value_context with_value(context& parent, std::string value, std::any key) {
  return value_context{parent, std::move(value), std::move(key)};
}

}  // namespace bongo::context
