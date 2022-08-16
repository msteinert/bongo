// Copyright The Go Authors.

#pragma once

#include <concepts>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <bongo/io.h>

namespace bongo::fmt {

// Width returns the value of the width option and whether it has been set.
template <typename T>
concept Width = requires (T v) {
  { v.width() } -> std::same_as<std::pair<int, bool>>;
};

// Precision returns the value fo the precision option and whether it has been
// set.
template <typename T>
concept Precision = requires (T v) {
  { v.precision() } -> std::same_as<std::pair<int, bool>>;
};

// Flag reports whether the flag c, a character, has been set.
template <typename T>
concept Flag = requires (T v, int c) {
  { v.flag(c) } -> std::same_as<bool>;
};

// State represents the printer state passed to custom formatters.
template <typename T>
concept State = requires {
  requires io::Writer<T> && Width<T> && Precision<T> && Flag<T>;
};

// Formatter is implemented by any value that has a format method. The
// implementation controls how state and rune are interpreted and may call
// fprintf(state) or fprint(state) etc. to generate its output.
template <typename T, typename U>
concept Formatter = requires (T f, U state, rune verb) {
  { f.format(state, verb) };
};

template <typename T, typename U> requires Formatter<T, U>
auto format(T& f, U& state, rune verb) -> void {
  f.format(state, verb);
}

template <typename T, typename U>
concept FormatterFunc = requires (T f, U state, rune verb) {
  { format(f, state, verb) };
};

}  // namespace bongo::fmt
