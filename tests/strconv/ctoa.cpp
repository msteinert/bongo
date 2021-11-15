// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Format complex", "[strconv]") {
  SECTION("double") {
    auto test_cases = std::vector<std::tuple<std::complex<double>, char, long, std::string_view>>{
      // A variety of signs
      {{1, 2}, 'g', -1, "(1+2i)"},
      {{3, -4}, 'g', -1, "(3-4i)"},
      {{-5, 6}, 'g', -1, "(-5+6i)"},
      {{-7, -8}, 'g', -1, "(-7-8i)"},

      // test that fmt and prec are working
      {{3.14159, 0.00123}, 'e', 3, "(3.142e+00+1.230e-03i)"},
      {{3.14159, 0.00123}, 'f', 3, "(3.142+0.001i)"},
      {{3.14159, 0.00123}, 'g', 3, "(3.14+0.00123i)"},

      // ensure bitSize rounding is working
      {{1.2345678901234567, 9.876543210987654}, 'f', -1, "(1.2345678901234567+9.876543210987654i)"},
    };
    for (auto [in, fmt, prec, exp] : test_cases) {
      CHECK(bongo::strconv::format(in, fmt, prec) == exp);
    }
  }
  SECTION("float") {
    auto test_cases = std::vector<std::tuple<std::complex<float>, char, long, std::string_view>>{
      // ensure bitSize rounding is working
      {{1.2345678901234567, 9.876543210987654}, 'f', -1, "(1.2345679+9.876543i)"},
    };
    for (auto [in, fmt, prec, exp] : test_cases) {
      CHECK(bongo::strconv::format(in, fmt, prec) == exp);
    }
  }
}
