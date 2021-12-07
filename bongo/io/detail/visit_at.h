// Copyright The Go Authors.

#pragma once

#include <functional>
#include <stdexcept>
#include <tuple>

namespace bongo::io::detail {

template <typename Function, typename Tuple, size_t N = 0>
auto visit_at(Function fn, Tuple& t, size_t i) {
  if (N == i) {
    return std::invoke(fn, std::get<N>(t));
  }
  if constexpr (N+1 < std::tuple_size_v<Tuple>) {
    return visit_at<Function, Tuple, N+1>(fn, t, i);
  }
  throw std::range_error{"index out of range: " + std::to_string(i)};
}

}  // namespace bongo::io::detail
