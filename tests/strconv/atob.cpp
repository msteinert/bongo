// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Parse bool", "[strconv]") {
  auto test_cases = std::vector<std::tuple<std::string_view, std::pair<bool, std::error_code>>>{
    {"",      {false, bongo::strconv::error::syntax}},
    {"asdf",  {false, bongo::strconv::error::syntax}},
    {"0",     {false, std::error_code{}}},
    {"f",     {false, std::error_code{}}},
    {"F",     {false, std::error_code{}}},
    {"FALSE", {false, std::error_code{}}},
    {"false", {false, std::error_code{}}},
    {"False", {false, std::error_code{}}},
    {"1",     {true,  std::error_code{}}},
    {"t",     {true,  std::error_code{}}},
    {"T",     {true,  std::error_code{}}},
    {"TRUE",  {true,  std::error_code{}}},
    {"true",  {true,  std::error_code{}}},
    {"True",  {true,  std::error_code{}}},
  };
  for (auto [in, exp] : test_cases) {
    CHECK(bongo::strconv::parse<bool>(in.begin(), in.end()) == exp);
  }
}

TEST_CASE("Format bool", "[strconv]") {
  auto test_cases = std::vector<std::tuple<bool, std::string_view>>{
    {true, "true"},
    {false , "false"},
  };
  for (auto [in, exp] : test_cases) {
    CHECK(bongo::strconv::format(in) == exp);
  }
}
