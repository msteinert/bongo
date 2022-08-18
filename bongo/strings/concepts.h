// Copyright The Go Authors.

#pragma once

#include <concepts>

namespace bongo::strings {

template <typename T>
concept IndexFunction = requires (T func, rune r) {
  { func(r) } -> std::same_as<bool>;
};

template <typename T>
concept MapFunction = requires (T func, rune r) {
  { func(r) } -> std::same_as<rune>;
};

}  // namespace bongo::strings
