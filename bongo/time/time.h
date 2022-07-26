// Copyright The Go Authors.

#pragma once

#include <type_traits>

namespace bongo::time {

template <typename>
struct is_clock : std::false_type {};

template <typename T>
  requires
    requires {
      typename T::rep;
      typename T::period;
      typename T::duration;
      typename T::time_point;
      T::is_steady;
      T::now();
    }
struct is_clock<T> : std::true_type {};

template <typename T>
constexpr bool is_clock_v = is_clock<T>::value;

template <typename T>
concept Clock = is_clock_v<std::remove_cvref_t<T>>;

}  // namespace bongo::time
