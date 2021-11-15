// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <iostream>
#include <iterator>
#include <string_view>

#include <catch2/catch.hpp>

namespace tests {

constexpr std::string_view basename(std::string_view s) {
  auto pos = s.find_last_of('/');
  return pos != std::string_view::npos ? s.substr(pos+1, s.size()) : s;
}

}  // namespace tests

#define MARK WARN("MARK: " << tests::basename(__FILE__) << ":" << __func__ << ":" << __LINE__)
