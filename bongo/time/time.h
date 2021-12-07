// Copyright The Go Authors.

#pragma once

#include <chrono>
#include <type_traits>

namespace bongo::time {

template <typename T>
concept Clock = std::chrono::is_clock_v<std::remove_cvref_t<T>>;

}  // namespace bongo::time
