// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Format floating point", "[strconv]") {
  auto test_cases = std::vector<std::tuple<double, char, int, std::string_view>>{
    {1, 'e', 5, "1.00000e+00"},
    {1, 'f', 5, "1.00000"},
    {1, 'g', 5, "1"},
    {1, 'g', -1, "1"},
    {1, 'x', -1, "0x1p+00"},
    {1, 'x', 5, "0x1.00000p+00"},
    {20, 'g', -1, "20"},
    {20, 'x', -1, "0x1.4p+04"},
    {1234567.8, 'g', -1, "1.2345678e+06"},
    {1234567.8, 'x', -1, "0x1.2d687cccccccdp+20"},
    {200000, 'g', -1, "200000"},
    {200000, 'x', -1, "0x1.86ap+17"},
    {200000, 'X', -1, "0X1.86AP+17"},
    {2000000, 'g', -1, "2e+06"},
    {1e10, 'g', -1, "1e+10"},

    // g conversion and zero suppression
    {400, 'g', 2, "4e+02"},
    {40, 'g', 2, "40"},
    {4, 'g', 2, "4"},
    {.4, 'g', 2, "0.4"},
    {.04, 'g', 2, "0.04"},
    {.004, 'g', 2, "0.004"},
    {.0004, 'g', 2, "0.0004"},
    {.00004, 'g', 2, "4e-05"},
    {.000004, 'g', 2, "4e-06"},

    {0, 'e', 5, "0.00000e+00"},
    {0, 'f', 5, "0.00000"},
    {0, 'g', 5, "0"},
    {0, 'g', -1, "0"},
    {0, 'x', 5, "0x0.00000p+00"},

    {-1, 'e', 5, "-1.00000e+00"},
    {-1, 'f', 5, "-1.00000"},
    {-1, 'g', 5, "-1"},
    {-1, 'g', -1, "-1"},

    {12, 'e', 5, "1.20000e+01"},
    {12, 'f', 5, "12.00000"},
    {12, 'g', 5, "12"},
    {12, 'g', -1, "12"},

    {123456700, 'e', 5, "1.23457e+08"},
    {123456700, 'f', 5, "123456700.00000"},
    {123456700, 'g', 5, "1.2346e+08"},
    {123456700, 'g', -1, "1.234567e+08"},

    {1.2345e6, 'e', 5, "1.23450e+06"},
    {1.2345e6, 'f', 5, "1234500.00000"},
    {1.2345e6, 'g', 5, "1.2345e+06"},

    // Round to even
    {1.2345e6, 'e', 3, "1.234e+06"},
    {1.2355e6, 'e', 3, "1.236e+06"},
    {1.2345, 'f', 3, "1.234"},
    {1.2355, 'f', 3, "1.236"},
    {1234567890123456.5, 'e', 15, "1.234567890123456e+15"},
    {1234567890123457.5, 'e', 15, "1.234567890123458e+15"},
    {108678236358137.625, 'g', -1, "1.0867823635813762e+14"},

    {1e23, 'e', 17, "9.99999999999999916e+22"},
    {1e23, 'f', 17, "99999999999999991611392.00000000000000000"},
    {1e23, 'g', 17, "9.9999999999999992e+22"},

    {1e23, 'e', -1, "1e+23"},
    {1e23, 'f', -1, "100000000000000000000000"},
    {1e23, 'g', -1, "1e+23"},

    {5e-304 / 1e20, 'g', -1, "5e-324"},   // avoid constant arithmetic
    {-5e-304 / 1e20, 'g', -1, "-5e-324"}, // avoid constant arithmetic

    {32, 'g', -1, "32"},
    {32, 'g', 0, "3e+01"},

    {100, 'x', -1, "0x1.9p+06"},
    {100, 'y', -1, "%y"},

    {NAN, 'g', -1, "NaN"},
    {std::numeric_limits<double>::signaling_NaN(), 'g', -1, "NaN"},
    {std::numeric_limits<double>::signaling_NaN(), 'g', -1, "NaN"},
    {std::numeric_limits<double>::infinity(), 'g', -1, "+Inf"},
    {-std::numeric_limits<double>::infinity(), 'g', -1, "-Inf"},

    {-1, 'b', -1, "-4503599627370496p-52"},

    // fixed bugs
    {0.9, 'f', 1, "0.9"},
    {0.09, 'f', 1, "0.1"},
    {0.0999, 'f', 1, "0.1"},
    {0.05, 'f', 1, "0.1"},
    {0.05, 'f', 0, "0"},
    {0.5, 'f', 1, "0.5"},
    {0.5, 'f', 0, "0"},
    {1.5, 'f', 0, "2"},

    // https://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
    {2.2250738585072012e-308, 'g', -1, "2.2250738585072014e-308"},
    // https://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
    {2.2250738585072011e-308, 'g', -1, "2.225073858507201e-308"},

    // Issue 2625.
    {383260575764816448, 'f', 0, "383260575764816448"},
    {383260575764816448, 'g', -1, "3.8326057576481645e+17"},

    // Issue 29491.
    {498484681984085570, 'f', -1, "498484681984085570"},
    {-5.8339553793802237e+23, 'g', -1, "-5.8339553793802237e+23"},

    // rounding
    {2.275555555555555, 'x', -1, "0x1.23456789abcdep+01"},
    {2.275555555555555, 'x', 0, "0x1p+01"},
    {2.275555555555555, 'x', 2, "0x1.23p+01"},
    {2.275555555555555, 'x', 16, "0x1.23456789abcde000p+01"},
    {2.275555555555555, 'x', 21, "0x1.23456789abcde00000000p+01"},
    {2.2755555510520935, 'x', -1, "0x1.2345678p+01"},
    {2.2755555510520935, 'x', 6, "0x1.234568p+01"},
    {2.275555431842804, 'x', -1, "0x1.2345668p+01"},
    {2.275555431842804, 'x', 6, "0x1.234566p+01"},
    {3.999969482421875, 'x', -1, "0x1.ffffp+01"},
    {3.999969482421875, 'x', 4, "0x1.ffffp+01"},
    {3.999969482421875, 'x', 3, "0x1.000p+02"},
    {3.999969482421875, 'x', 2, "0x1.00p+02"},
    {3.999969482421875, 'x', 1, "0x1.0p+02"},
    {3.999969482421875, 'x', 0, "0x1p+02"},
  };
  for (auto [in, fmt, prec, exp] : test_cases) {
    CAPTURE(in, fmt, prec);
    CHECK(bongo::strconv::format(in, fmt, prec) == exp);
    auto in_f = static_cast<float>(in);
    if (static_cast<double>(in_f) == in && fmt != 'b') {
      CAPTURE(in_f);
      CHECK(bongo::strconv::format(in_f, fmt, prec) == exp);
    }
  }
}

TEST_CASE("Ryu", "[strconv]") {
  SECTION("Multiply by log2 over log10") {
    for (long x = -1600; x <= +1600; ++x) {
      auto imath = bongo::strconv::detail::mul_by_log2_log10(x);
      auto fmath = static_cast<long>(std::floor(static_cast<double>(x) * M_LN2 / M_LN10));
      CAPTURE(x);
      CHECK(imath == fmath);
    }
  }
  SECTION("Multiply by log10 over log2") {
    for (long x = -500; x <= +500; ++x) {
      auto imath = bongo::strconv::detail::mul_by_log10_log2(x);
      auto fmath = static_cast<long>(std::floor(static_cast<double>(x) * M_LN10 / M_LN2));
      CAPTURE(x);
      CHECK(imath == fmath);
    }
  }
}
