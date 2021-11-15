// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <complex>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Parse complex", "[strconv]") {
  auto nan = std::numeric_limits<double>::quiet_NaN();
  auto inf = std::numeric_limits<double>::infinity();

  auto infp0 = std::complex<double>{+inf, 0};
  auto infm0 = std::complex<double>{-inf, 0};
  auto inf0p = std::complex<double>{0, +inf};
  auto inf0m = std::complex<double>{0, -inf};

  auto infpp = std::complex<double>{+inf, +inf};
  auto infpm = std::complex<double>{+inf, -inf};
  auto infmp = std::complex<double>{-inf, +inf};
  auto infmm = std::complex<double>{-inf, -inf};

  auto test_cases = std::vector<std::tuple<std::string_view, std::complex<double>, std::error_code>>{
    // Clearly invalid
    {"", {0, 0}, bongo::strconv::error::syntax},
    {"", 0, bongo::strconv::error::syntax},
    {" ", 0, bongo::strconv::error::syntax},
    {"(", 0, bongo::strconv::error::syntax},
    {")", 0, bongo::strconv::error::syntax},
    {"i", 0, bongo::strconv::error::syntax},
    {"+i", 0, bongo::strconv::error::syntax},
    {"-i", 0, bongo::strconv::error::syntax},
    {"1I", 0, bongo::strconv::error::syntax},
    {"10  + 5i", 0, bongo::strconv::error::syntax},
    {"3+", 0, bongo::strconv::error::syntax},
    {"3+5", 0, bongo::strconv::error::syntax},
    {"3+5+5i", 0, bongo::strconv::error::syntax},

    // Parentheses
    {"()", 0, bongo::strconv::error::syntax},
    {"(i)", 0, bongo::strconv::error::syntax},
    {"(0)", 0, std::error_code{}},
    {"(1i)", {0, 1}, std::error_code{}},
    {"(3.0+5.5i)", {3.0, 5.5}, std::error_code{}},
    {"(1)+1i", 0, bongo::strconv::error::syntax},
    {"(3.0+5.5i", 0, bongo::strconv::error::syntax},
    {"3.0+5.5i)", 0, bongo::strconv::error::syntax},

    // NaNs
    {"NaN", {nan, 0}, std::error_code{}},
    {"NaNi", {0, nan}, std::error_code{}},
    {"nan+nAni", {nan, nan}, std::error_code{}},
    {"+NaN", 0, bongo::strconv::error::syntax},
    {"-NaN", 0, bongo::strconv::error::syntax},
    {"NaN-NaNi", 0, bongo::strconv::error::syntax},

    // Infs
    {"Inf", infp0, std::error_code{}},
    {"+inf", infp0, std::error_code{}},
    {"-inf", infm0, std::error_code{}},
    {"Infinity", infp0, std::error_code{}},
    {"+INFINITY", infp0, std::error_code{}},
    {"-infinity", infm0, std::error_code{}},
    {"+infi", inf0p, std::error_code{}},
    {"0-infinityi", inf0m, std::error_code{}},
    {"Inf+Infi", infpp, std::error_code{}},
    {"+Inf-Infi", infpm, std::error_code{}},
    {"-Infinity+Infi", infmp, std::error_code{}},
    {"inf-inf", 0, bongo::strconv::error::syntax},

    // Zeros
    {"0", 0, std::error_code{}},
    {"0i", 0, std::error_code{}},
    {"-0.0i", 0, std::error_code{}},
    {"0+0.0i", 0, std::error_code{}},
    {"0e+0i", 0, std::error_code{}},
    {"0e-0+0i", 0, std::error_code{}},
    {"-0.0-0.0i", 0, std::error_code{}},
    {"0e+012345", 0, std::error_code{}},
    {"0x0p+012345i", 0, std::error_code{}},
    {"0x0.00p-012345i", 0, std::error_code{}},
    {"+0e-0+0e-0i", 0, std::error_code{}},
    {"0e+0+0e+0i", 0, std::error_code{}},
    {"-0e+0-0e+0i", 0, std::error_code{}},

    // Zeros
    {"0", 0, std::error_code{}},
    {"0i", 0, std::error_code{}},
    {"-0.0i", 0, std::error_code{}},
    {"0+0.0i", 0, std::error_code{}},
    {"0e+0i", 0, std::error_code{}},
    {"0e-0+0i", 0, std::error_code{}},
    {"-0.0-0.0i", 0, std::error_code{}},
    {"0e+012345", 0, std::error_code{}},
    {"0x0p+012345i", 0, std::error_code{}},
    {"0x0.00p-012345i", 0, std::error_code{}},
    {"+0e-0+0e-0i", 0, std::error_code{}},
    {"0e+0+0e+0i", 0, std::error_code{}},
    {"-0e+0-0e+0i", 0, std::error_code{}},

    // Regular non-zeroes
    {"0.1", 0.1, std::error_code{}},
    {"0.1i", {0, 0.1}, std::error_code{}},
    {"0.123", 0.123, std::error_code{}},
    {"0.123i", {0, 0.123}, std::error_code{}},
    {"0.123+0.123i", {0.123, 0.123}, std::error_code{}},
    {"99", 99, std::error_code{}},
    {"+99", 99, std::error_code{}},
    {"-99", -99, std::error_code{}},
    {"+1i", {0, 1}, std::error_code{}},
    {"-1i", {0, -1}, std::error_code{}},
    {"+3+1i", {3, 1}, std::error_code{}},
    {"30+3i", {30, 3}, std::error_code{}},
    {"+3e+3-3e+3i", {3e+3, -3e+3}, std::error_code{}},
    {"+3e+3+3e+3i", {3e+3, 3e+3}, std::error_code{}},
    {"+3e+3+3e+3i+", 0, bongo::strconv::error::syntax},

    // Separators
    {"0.1", 0.1, std::error_code{}},
    {"0.1i", {0, 0.1}, std::error_code{}},
    {"0.1'2'3", 0.123, std::error_code{}},
    {"+0x3p3i", {0, 0x3p3}, std::error_code{}},
    {"0'0+0x0p0i", 0, std::error_code{}},
    {"0x1'0.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, std::error_code{}},
    {"+0x1'0.3p-8+0x3'0p3i", {0x10.3p-8, 0x30p3}, std::error_code{}},
    {"0x1'0.3p+8-0x3p3i", {0x10.3p+8, -0x3p3}, std::error_code{}},

    // Hexadecimals
    {"0x10.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, std::error_code{}},
    {"+0x10.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, std::error_code{}},
    {"0x10.3p+8-0x3p3i", {0x10.3p+8, -0x3p3}, std::error_code{}},
    {"0x1p0", 1, std::error_code{}},
    {"0x1p1", 2, std::error_code{}},
    {"0x1p-1", 0.5, std::error_code{}},
    {"0x1ep-1", 15, std::error_code{}},
    {"-0x1ep-1", -15, std::error_code{}},
    {"-0x2p3", -16, std::error_code{}},
    {"0x1e2", 0, bongo::strconv::error::syntax},
    {"1p2", 0, bongo::strconv::error::syntax},
    {"0x1e2i", 0, bongo::strconv::error::syntax},

    // bongo::strconv::error::range
    // next float64 - too large
    {"+0x1p1024", infp0, bongo::strconv::error::range},
    {"-0x1p1024", infm0, bongo::strconv::error::range},
    {"+0x1p1024i", inf0p, bongo::strconv::error::range},
    {"-0x1p1024i", inf0m, bongo::strconv::error::range},
    {"+0x1p1024+0x1p1024i", infpp, bongo::strconv::error::range},
    {"+0x1p1024-0x1p1024i", infpm, bongo::strconv::error::range},
    {"-0x1p1024+0x1p1024i", infmp, bongo::strconv::error::range},
    {"-0x1p1024-0x1p1024i", infmm, bongo::strconv::error::range},
    // the border is ...158079
    // borderline - okay
    {"+0x1.fffffffffffff7fffp1023+0x1.fffffffffffff7fffp1023i", {1.7976931348623157e+308, 1.7976931348623157e+308}, std::error_code{}},
    {"+0x1.fffffffffffff7fffp1023-0x1.fffffffffffff7fffp1023i", {1.7976931348623157e+308, -1.7976931348623157e+308}, std::error_code{}},
    {"-0x1.fffffffffffff7fffp1023+0x1.fffffffffffff7fffp1023i", {-1.7976931348623157e+308, 1.7976931348623157e+308}, std::error_code{}},
    {"-0x1.fffffffffffff7fffp1023-0x1.fffffffffffff7fffp1023i", {-1.7976931348623157e+308, -1.7976931348623157e+308}, std::error_code{}},
    // borderline - too large
    {"+0x1.fffffffffffff8p1023", infp0, bongo::strconv::error::range},
    {"-0x1fffffffffffff.8p+971", infm0, bongo::strconv::error::range},
    {"+0x1.fffffffffffff8p1023i", inf0p, bongo::strconv::error::range},
    {"-0x1fffffffffffff.8p+971i", inf0m, bongo::strconv::error::range},
    {"+0x1.fffffffffffff8p1023+0x1.fffffffffffff8p1023i", infpp, bongo::strconv::error::range},
    {"+0x1.fffffffffffff8p1023-0x1.fffffffffffff8p1023i", infpm, bongo::strconv::error::range},
    {"-0x1fffffffffffff.8p+971+0x1fffffffffffff.8p+971i", infmp, bongo::strconv::error::range},
    {"-0x1fffffffffffff8p+967-0x1fffffffffffff8p+967i", infmm, bongo::strconv::error::range},
    // a little too large
    {"1e308+1e308i", {1e+308, 1e+308}, std::error_code{}},
    {"2e308+2e308i", infpp, bongo::strconv::error::range},
    {"1e309+1e309i", infpp, bongo::strconv::error::range},
    {"0x1p1025+0x1p1025i", infpp, bongo::strconv::error::range},
    {"2e308", infp0, bongo::strconv::error::range},
    {"1e309", infp0, bongo::strconv::error::range},
    {"0x1p1025", infp0, bongo::strconv::error::range},
    {"2e308i", inf0p, bongo::strconv::error::range},
    {"1e309i", inf0p, bongo::strconv::error::range},
    {"0x1p1025i", inf0p, bongo::strconv::error::range},
    // way too large
    {"+1e310+1e310i", infpp, bongo::strconv::error::range},
    {"+1e310-1e310i", infpm, bongo::strconv::error::range},
    {"-1e310+1e310i", infmp, bongo::strconv::error::range},
    {"-1e310-1e310i", infmm, bongo::strconv::error::range},
    // under/overflow exponent
    {"1e-4294967296", 0, std::error_code{}},
    {"1e-4294967296i", 0, std::error_code{}},
    {"1e-4294967296+1i", {0, 1}, std::error_code{}},
    {"1+1e-4294967296i", 1, std::error_code{}},
    {"1e-4294967296+1e-4294967296i", 0, std::error_code{}},
    {"1e+4294967296", infp0, bongo::strconv::error::range},
    {"1e+4294967296i", inf0p, bongo::strconv::error::range},
    {"1e+4294967296+1e+4294967296i", infpp, bongo::strconv::error::range},
    {"1e+4294967296-1e+4294967296i", infpm, bongo::strconv::error::range},
  };
  for (auto [in, exp, exp_err] : test_cases) {
    CAPTURE(in);
    auto [c, err] = bongo::strconv::parse<std::complex<double>>(in);
    if (!std::isnan(exp.real())) {
      INFO("real")
      CHECK(c.real() == exp.real());
    } else {
      INFO("real NaN")
      CHECK(std::isnan(c.real()));
    }
    if (!std::isnan(exp.imag())) {
      INFO("imag")
      CHECK(c.imag() == exp.imag());
    } else {
      INFO("imag NaN")
      CHECK(std::isnan(c.imag()));
    }
    CHECK(err == exp_err);
  }
}
