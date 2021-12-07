// Copyright The Go Authors.

#include <complex>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/strconv.h"

using namespace std::string_view_literals;

namespace bongo::strconv {

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
    {"", {0, 0}, error::syntax},
    {"", 0, error::syntax},
    {" ", 0, error::syntax},
    {"(", 0, error::syntax},
    {")", 0, error::syntax},
    {"i", 0, error::syntax},
    {"+i", 0, error::syntax},
    {"-i", 0, error::syntax},
    {"1I", 0, error::syntax},
    {"10  + 5i", 0, error::syntax},
    {"3+", 0, error::syntax},
    {"3+5", 0, error::syntax},
    {"3+5+5i", 0, error::syntax},

    // Parentheses
    {"()", 0, error::syntax},
    {"(i)", 0, error::syntax},
    {"(0)", 0, nil},
    {"(1i)", {0, 1}, nil},
    {"(3.0+5.5i)", {3.0, 5.5}, nil},
    {"(1)+1i", 0, error::syntax},
    {"(3.0+5.5i", 0, error::syntax},
    {"3.0+5.5i)", 0, error::syntax},

    // NaNs
    {"NaN", {nan, 0}, nil},
    {"NaNi", {0, nan}, nil},
    {"nan+nAni", {nan, nan}, nil},
    {"+NaN", 0, error::syntax},
    {"-NaN", 0, error::syntax},
    {"NaN-NaNi", 0, error::syntax},

    // Infs
    {"Inf", infp0, nil},
    {"+inf", infp0, nil},
    {"-inf", infm0, nil},
    {"Infinity", infp0, nil},
    {"+INFINITY", infp0, nil},
    {"-infinity", infm0, nil},
    {"+infi", inf0p, nil},
    {"0-infinityi", inf0m, nil},
    {"Inf+Infi", infpp, nil},
    {"+Inf-Infi", infpm, nil},
    {"-Infinity+Infi", infmp, nil},
    {"inf-inf", 0, error::syntax},

    // Zeros
    {"0", 0, nil},
    {"0i", 0, nil},
    {"-0.0i", 0, nil},
    {"0+0.0i", 0, nil},
    {"0e+0i", 0, nil},
    {"0e-0+0i", 0, nil},
    {"-0.0-0.0i", 0, nil},
    {"0e+012345", 0, nil},
    {"0x0p+012345i", 0, nil},
    {"0x0.00p-012345i", 0, nil},
    {"+0e-0+0e-0i", 0, nil},
    {"0e+0+0e+0i", 0, nil},
    {"-0e+0-0e+0i", 0, nil},

    // Zeros
    {"0", 0, nil},
    {"0i", 0, nil},
    {"-0.0i", 0, nil},
    {"0+0.0i", 0, nil},
    {"0e+0i", 0, nil},
    {"0e-0+0i", 0, nil},
    {"-0.0-0.0i", 0, nil},
    {"0e+012345", 0, nil},
    {"0x0p+012345i", 0, nil},
    {"0x0.00p-012345i", 0, nil},
    {"+0e-0+0e-0i", 0, nil},
    {"0e+0+0e+0i", 0, nil},
    {"-0e+0-0e+0i", 0, nil},

    // Regular non-zeroes
    {"0.1", 0.1, nil},
    {"0.1i", {0, 0.1}, nil},
    {"0.123", 0.123, nil},
    {"0.123i", {0, 0.123}, nil},
    {"0.123+0.123i", {0.123, 0.123}, nil},
    {"99", 99, nil},
    {"+99", 99, nil},
    {"-99", -99, nil},
    {"+1i", {0, 1}, nil},
    {"-1i", {0, -1}, nil},
    {"+3+1i", {3, 1}, nil},
    {"30+3i", {30, 3}, nil},
    {"+3e+3-3e+3i", {3e+3, -3e+3}, nil},
    {"+3e+3+3e+3i", {3e+3, 3e+3}, nil},
    {"+3e+3+3e+3i+", 0, error::syntax},

    // Separators
    {"0.1", 0.1, nil},
    {"0.1i", {0, 0.1}, nil},
    {"0.1'2'3", 0.123, nil},
    {"+0x3p3i", {0, 0x3p3}, nil},
    {"0'0+0x0p0i", 0, nil},
    {"0x1'0.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, nil},
    {"+0x1'0.3p-8+0x3'0p3i", {0x10.3p-8, 0x30p3}, nil},
    {"0x1'0.3p+8-0x3p3i", {0x10.3p+8, -0x3p3}, nil},

    // Hexadecimals
    {"0x10.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, nil},
    {"+0x10.3p-8+0x3p3i", {0x10.3p-8, 0x3p3}, nil},
    {"0x10.3p+8-0x3p3i", {0x10.3p+8, -0x3p3}, nil},
    {"0x1p0", 1, nil},
    {"0x1p1", 2, nil},
    {"0x1p-1", 0.5, nil},
    {"0x1ep-1", 15, nil},
    {"-0x1ep-1", -15, nil},
    {"-0x2p3", -16, nil},
    {"0x1e2", 0, error::syntax},
    {"1p2", 0, error::syntax},
    {"0x1e2i", 0, error::syntax},

    // error::range
    // next float64 - too large
    {"+0x1p1024", infp0, error::range},
    {"-0x1p1024", infm0, error::range},
    {"+0x1p1024i", inf0p, error::range},
    {"-0x1p1024i", inf0m, error::range},
    {"+0x1p1024+0x1p1024i", infpp, error::range},
    {"+0x1p1024-0x1p1024i", infpm, error::range},
    {"-0x1p1024+0x1p1024i", infmp, error::range},
    {"-0x1p1024-0x1p1024i", infmm, error::range},
    // the border is ...158079
    // borderline - okay
    {"+0x1.fffffffffffff7fffp1023+0x1.fffffffffffff7fffp1023i", {1.7976931348623157e+308, 1.7976931348623157e+308}, nil},
    {"+0x1.fffffffffffff7fffp1023-0x1.fffffffffffff7fffp1023i", {1.7976931348623157e+308, -1.7976931348623157e+308}, nil},
    {"-0x1.fffffffffffff7fffp1023+0x1.fffffffffffff7fffp1023i", {-1.7976931348623157e+308, 1.7976931348623157e+308}, nil},
    {"-0x1.fffffffffffff7fffp1023-0x1.fffffffffffff7fffp1023i", {-1.7976931348623157e+308, -1.7976931348623157e+308}, nil},
    // borderline - too large
    {"+0x1.fffffffffffff8p1023", infp0, error::range},
    {"-0x1fffffffffffff.8p+971", infm0, error::range},
    {"+0x1.fffffffffffff8p1023i", inf0p, error::range},
    {"-0x1fffffffffffff.8p+971i", inf0m, error::range},
    {"+0x1.fffffffffffff8p1023+0x1.fffffffffffff8p1023i", infpp, error::range},
    {"+0x1.fffffffffffff8p1023-0x1.fffffffffffff8p1023i", infpm, error::range},
    {"-0x1fffffffffffff.8p+971+0x1fffffffffffff.8p+971i", infmp, error::range},
    {"-0x1fffffffffffff8p+967-0x1fffffffffffff8p+967i", infmm, error::range},
    // a little too large
    {"1e308+1e308i", {1e+308, 1e+308}, nil},
    {"2e308+2e308i", infpp, error::range},
    {"1e309+1e309i", infpp, error::range},
    {"0x1p1025+0x1p1025i", infpp, error::range},
    {"2e308", infp0, error::range},
    {"1e309", infp0, error::range},
    {"0x1p1025", infp0, error::range},
    {"2e308i", inf0p, error::range},
    {"1e309i", inf0p, error::range},
    {"0x1p1025i", inf0p, error::range},
    // way too large
    {"+1e310+1e310i", infpp, error::range},
    {"+1e310-1e310i", infpm, error::range},
    {"-1e310+1e310i", infmp, error::range},
    {"-1e310-1e310i", infmm, error::range},
    // under/overflow exponent
    {"1e-4294967296", 0, nil},
    {"1e-4294967296i", 0, nil},
    {"1e-4294967296+1i", {0, 1}, nil},
    {"1+1e-4294967296i", 1, nil},
    {"1e-4294967296+1e-4294967296i", 0, nil},
    {"1e+4294967296", infp0, error::range},
    {"1e+4294967296i", inf0p, error::range},
    {"1e+4294967296+1e+4294967296i", infpp, error::range},
    {"1e+4294967296-1e+4294967296i", infpm, error::range},
  };
  for (auto [in, exp, exp_err] : test_cases) {
    CAPTURE(in);
    auto [c, err] = parse<std::complex<double>>(in);
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

}  // namespace bongo::strconv
