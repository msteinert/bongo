// Copyright The Go Authors.

#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>

namespace bongo::fmt::detail {

template <typename Function, typename Tuple, size_t N = 0>
auto visit_at(Function fn, Tuple& t, size_t i) -> void {
  if (N == i) {
    std::invoke(fn, std::get<N>(t));
    return;
  }
  if constexpr (N+1 < std::tuple_size_v<Tuple>) {
    visit_at<Function, Tuple, N+1>(fn, t, i);
    return;
  }
  throw std::range_error{"index out of range: " + std::to_string(i)};
}

template <typename Function, typename Tuple, size_t N = 0>
auto visit_at(Function fn, Tuple const& t, size_t i) -> void {
  if (N == i) {
    std::invoke(fn, std::get<N>(t));
    return;
  }
  if constexpr (N+1 < std::tuple_size_v<Tuple>) {
    visit_at<Function, Tuple, N+1>(fn, t, i);
    return;
  }
  throw std::range_error{"index out of range: " + std::to_string(i)};
}

template <typename Function, typename... Args>
auto visit_arg(Function fn, size_t i, Args... args) -> void {
  if constexpr (sizeof... (args) > 0) {
    visit_at(fn, std::forward_as_tuple(args...), i);
  }
}

}  // namespace bongo::fmt::detail
