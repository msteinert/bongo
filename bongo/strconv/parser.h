// Copyright The Go Authors.

#pragma once

#include <array>
#include <iterator>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/strconv/parser.h>

namespace bongo::strconv {

template <typename T, std::input_iterator InputIt, typename = void>
struct parser {
  constexpr std::pair<T, std::error_code> operator()(InputIt begin, InputIt end);
};

}  // namespace bongo::strconv
