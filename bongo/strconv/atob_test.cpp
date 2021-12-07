// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/strconv.h"

using namespace std::string_view_literals;

namespace bongo::strconv {

TEST_CASE("Parse bool", "[strconv]") {
  auto test_cases = std::vector<std::tuple<std::string_view, std::pair<bool, std::error_code>>>{
    {"",      {false, error::syntax}},
    {"asdf",  {false, error::syntax}},
    {"0",     {false, nil}},
    {"f",     {false, nil}},
    {"F",     {false, nil}},
    {"FALSE", {false, nil}},
    {"false", {false, nil}},
    {"False", {false, nil}},
    {"1",     {true,  nil}},
    {"t",     {true,  nil}},
    {"T",     {true,  nil}},
    {"TRUE",  {true,  nil}},
    {"true",  {true,  nil}},
    {"True",  {true,  nil}},
  };
  for (auto [in, exp] : test_cases) {
    CHECK(parse<bool>(in.begin(), in.end()) == exp);
  }
}

TEST_CASE("Format bool", "[strconv]") {
  auto test_cases = std::vector<std::tuple<bool, std::string_view>>{
    {true, "true"},
    {false , "false"},
  };
  for (auto [in, exp] : test_cases) {
    CHECK(format(in) == exp);
  }
}

}  // namespace bongo::strconv
