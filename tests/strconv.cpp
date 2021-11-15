// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <complex>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

TEST_CASE("Test strconv", "[strconv]") {
  SECTION("Parse bool") {
    auto [value, err] = bongo::strconv::parse<bool>("True");
    REQUIRE(err == nullptr);
    REQUIRE(value == true);
  }
  SECTION("Format bool") {
    REQUIRE(bongo::strconv::format(true) == "true");
    REQUIRE(bongo::strconv::format(false) == "false");
  }
  SECTION("Parse integer") {
    auto [value, err] = bongo::strconv::parse<int>("123");
    REQUIRE(err == nullptr);
    REQUIRE(value == 123);
  }
  SECTION("Format integer") {
    REQUIRE(bongo::strconv::format(100) == "100");
    REQUIRE(bongo::strconv::format(-100) == "-100");
  }
}
