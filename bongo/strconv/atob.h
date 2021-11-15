// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <array>
#include <iterator>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/strconv/error.h>
#include <bongo/strconv/parser.h>

namespace bongo::strconv {

static constexpr std::array<std::string_view, 6> true_strings{"1", "t", "T", "true", "TRUE", "True"};
static constexpr std::array<std::string_view, 6> false_strings{"0", "f", "F", "false", "FALSE", "False"};

template <typename InputIt>
struct parser<bool, InputIt> {
  constexpr std::pair<bool, std::error_code> operator()(InputIt begin, InputIt end) {
    for (auto s : true_strings) {
      if (std::equal(begin, end, std::begin(s), std::end(s))) {
        return {true, std::error_code{}};
      }
    }
    for (auto s : false_strings) {
      if (std::equal(begin, end, std::begin(s), std::end(s))) {
        return {false, std::error_code{}};
      }
    }
    return {false, error::syntax};
  }
};

}  // namespace bongo::strconv
