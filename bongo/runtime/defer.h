// Copyright The Go Authors.

#pragma once

#include <utility>

namespace bongo::runtime {

template <typename Fn>
class deferred_action {
  Fn fn_;

 public:
  deferred_action(Fn fn)
      : fn_{std::move(fn)} {}

  ~deferred_action() {
    try { fn_(); } catch (...) {};
  }

  deferred_action(deferred_action&& other)
      : fn_{std::move(other.fn_)} {}

  deferred_action(deferred_action const& other) = delete;
  deferred_action& operator=(deferred_action const& other) = delete;
  deferred_action& operator=(deferred_action&& other) = delete;
};

template <typename Fn>
[[nodiscard]] deferred_action<Fn> defer(Fn const& fn) {
  return deferred_action<Fn>(fn);
}

template <typename Fn>
[[nodiscard]] deferred_action<Fn> defer(Fn&& fn) {
  return deferred_action<Fn>(std::forward<Fn>(fn));
}

}  // namespace bongo::runtime
