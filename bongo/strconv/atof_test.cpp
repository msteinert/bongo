// Copyright The Go Authors.

#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/strconv.h"

using namespace std::string_view_literals;
using namespace std::string_literals;

namespace bongo::strconv {

TEST_CASE("Parse floating point", "[strconv]") {
  SECTION("double") {
    auto test_cases = std::vector<std::tuple<std::string, std::string_view, std::error_code>>{
      {"", "0", error::syntax},
      {"1", "1", nil},
      {"+1", "1", nil},
      {"1x", "0", error::syntax},
      {"1.1.", "0", error::syntax},
      {"1e23", "1e+23", nil},
      {"1E23", "1e+23", nil},
      {"100000000000000000000000", "1e+23", nil},
      {"1e-100", "1e-100", nil},
      {"123456700", "1.234567e+08", nil},
      {"99999999999999974834176", "9.999999999999997e+22", nil},
      {"100000000000000000000001", "1.0000000000000001e+23", nil},
      {"100000000000000008388608", "1.0000000000000001e+23", nil},
      {"100000000000000016777215", "1.0000000000000001e+23", nil},
      {"100000000000000016777216", "1.0000000000000003e+23", nil},
      {"-1", "-1", nil},
      {"-0.1", "-0.1", nil},
      {"-0", "-0", nil},
      {"1e-20", "1e-20", nil},
      {"625e-3", "0.625", nil},

      // Hexadecimal floating-point.
      {"0x1p0", "1", nil},
      {"0x1p1", "2", nil},
      {"0x1p-1", "0.5", nil},
      {"0x1ep-1", "15", nil},
      {"-0x1ep-1", "-15", nil},
      {"-0x1'ep-1", "-15", nil},
      {"0x1p-200", "6.223015277861142e-61", nil},
      {"0x1p200", "1.6069380442589903e+60", nil},
      {"0x1fFe2.p0", "131042", nil},
      {"0x1fFe2.P0", "131042", nil},
      {"-0x2p3", "-16", nil},
      {"0x0.fp4", "15", nil},
      {"0x0.fp0", "0.9375", nil},
      {"0x1e2", "0", error::syntax},
      {"1p2", "0", error::syntax},

      // zeros
      {"0", "0", nil},
      {"0e0", "0", nil},
      {"-0e0", "-0", nil},
      {"+0e0", "0", nil},
      {"0e-0", "0", nil},
      {"-0e-0", "-0", nil},
      {"+0e-0", "0", nil},
      {"0e+0", "0", nil},
      {"-0e+0", "-0", nil},
      {"+0e+0", "0", nil},
      {"0e+01234567890123456789", "0", nil},
      {"0.00e-01234567890123456789", "0", nil},
      {"-0e+01234567890123456789", "-0", nil},
      {"-0.00e-01234567890123456789", "-0", nil},
      {"0x0p+01234567890123456789", "0", nil},
      {"0x0.00p-01234567890123456789", "0", nil},
      {"-0x0p+01234567890123456789", "-0", nil},
      {"-0x0.00p-01234567890123456789", "-0", nil},

      {"0e291", "0", nil}, // issue 15364
      {"0e292", "0", nil}, // issue 15364
      {"0e347", "0", nil}, // issue 15364
      {"0e348", "0", nil}, // issue 15364
      {"-0e291", "-0", nil},
      {"-0e292", "-0", nil},
      {"-0e347", "-0", nil},
      {"-0e348", "-0", nil},
      {"0x0p126", "0", nil},
      {"0x0p127", "0", nil},
      {"0x0p128", "0", nil},
      {"0x0p129", "0", nil},
      {"0x0p130", "0", nil},
      {"0x0p1022", "0", nil},
      {"0x0p1023", "0", nil},
      {"0x0p1024", "0", nil},
      {"0x0p1025", "0", nil},
      {"0x0p1026", "0", nil},
      {"-0x0p126", "-0", nil},
      {"-0x0p127", "-0", nil},
      {"-0x0p128", "-0", nil},
      {"-0x0p129", "-0", nil},
      {"-0x0p130", "-0", nil},
      {"-0x0p1022", "-0", nil},
      {"-0x0p1023", "-0", nil},
      {"-0x0p1024", "-0", nil},
      {"-0x0p1025", "-0", nil},
      {"-0x0p1026", "-0", nil},

      // NaNs
      {"nan", "NaN", nil},
      {"NaN", "NaN", nil},
      {"NAN", "NaN", nil},

      // Infs
      {"inf", "+Inf", nil},
      {"-Inf", "-Inf", nil},
      {"+INF", "+Inf", nil},
      {"-Infinity", "-Inf", nil},
      {"+INFINITY", "+Inf", nil},
      {"Infinity", "+Inf", nil},

      // largest float64
      {"1.7976931348623157e308", "1.7976931348623157e+308", nil},
      {"-1.7976931348623157e308", "-1.7976931348623157e+308", nil},
      {"0x1.fffffffffffffp1023", "1.7976931348623157e+308", nil},
      {"-0x1.fffffffffffffp1023", "-1.7976931348623157e+308", nil},
      {"0x1fffffffffffffp+971", "1.7976931348623157e+308", nil},
      {"-0x1fffffffffffffp+971", "-1.7976931348623157e+308", nil},
      {"0x.1fffffffffffffp1027", "1.7976931348623157e+308", nil},
      {"-0x.1fffffffffffffp1027", "-1.7976931348623157e+308", nil},

      // next float64 - too large
      {"1.7976931348623159e308", "+Inf", error::range},
      {"-1.7976931348623159e308", "-Inf", error::range},
      {"0x1p1024", "+Inf", error::range},
      {"-0x1p1024", "-Inf", error::range},
      {"0x2p1023", "+Inf", error::range},
      {"-0x2p1023", "-Inf", error::range},
      {"0x.1p1028", "+Inf", error::range},
      {"-0x.1p1028", "-Inf", error::range},
      {"0x.2p1027", "+Inf", error::range},
      {"-0x.2p1027", "-Inf", error::range},

      // the border is ...158079
      // borderline - okay
      {"1.7976931348623158e308", "1.7976931348623157e+308", nil},
      {"-1.7976931348623158e308", "-1.7976931348623157e+308", nil},
      {"0x1.fffffffffffff7fffp1023", "1.7976931348623157e+308", nil},
      {"-0x1.fffffffffffff7fffp1023", "-1.7976931348623157e+308", nil},
      // borderline - too large
      {"1.797693134862315808e308", "+Inf", error::range},
      {"-1.797693134862315808e308", "-Inf", error::range},
      {"0x1.fffffffffffff8p1023", "+Inf", error::range},
      {"-0x1.fffffffffffff8p1023", "-Inf", error::range},
      {"0x1fffffffffffff.8p+971", "+Inf", error::range},
      {"-0x1fffffffffffff8p+967", "-Inf", error::range},
      {"0x.1fffffffffffff8p1027", "+Inf", error::range},
      {"-0x.1fffffffffffff9p1027", "-Inf", error::range},

      // a little too large
      {"1e308", "1e+308", nil},
      {"2e308", "+Inf", error::range},
      {"1e309", "+Inf", error::range},
      {"0x1p1025", "+Inf", error::range},

      // way too large
      {"1e310", "+Inf", error::range},
      {"-1e310", "-Inf", error::range},
      {"1e400", "+Inf", error::range},
      {"-1e400", "-Inf", error::range},
      {"1e400000", "+Inf", error::range},
      {"-1e400000", "-Inf", error::range},
      {"0x1p1030", "+Inf", error::range},
      {"0x1p2000", "+Inf", error::range},
      {"0x1p2000000000", "+Inf", error::range},
      {"-0x1p1030", "-Inf", error::range},
      {"-0x1p2000", "-Inf", error::range},
      {"-0x1p2000000000", "-Inf", error::range},

      // denormalized
      {"1e-305", "1e-305", nil},
      {"1e-306", "1e-306", nil},
      {"1e-307", "1e-307", nil},
      {"1e-308", "1e-308", nil},
      {"1e-309", "1e-309", nil},
      {"1e-310", "1e-310", nil},
      {"1e-322", "1e-322", nil},
      // smallest denormal
      {"5e-324", "5e-324", nil},
      {"4e-324", "5e-324", nil},
      {"3e-324", "5e-324", nil},
      // too small
      {"2e-324", "0", nil},
      // way too small
      {"1e-350", "0", nil},
      {"1e-400000", "0", nil},

      // Near denormals and denormals.
      {"0x2.00000000000000p-1010", "1.8227805048890994e-304", nil}, // 0x00e0000000000000
      {"0x1.fffffffffffff0p-1010", "1.8227805048890992e-304", nil}, // 0x00dfffffffffffff
      {"0x1.fffffffffffff7p-1010", "1.8227805048890992e-304", nil}, // rounded down
      {"0x1.fffffffffffff8p-1010", "1.8227805048890994e-304", nil}, // rounded up
      {"0x1.fffffffffffff9p-1010", "1.8227805048890994e-304", nil}, // rounded up

      {"0x2.00000000000000p-1022", "4.450147717014403e-308", nil},  // 0x0020000000000000
      {"0x1.fffffffffffff0p-1022", "4.4501477170144023e-308", nil}, // 0x001fffffffffffff
      {"0x1.fffffffffffff7p-1022", "4.4501477170144023e-308", nil}, // rounded down
      {"0x1.fffffffffffff8p-1022", "4.450147717014403e-308", nil},  // rounded up
      {"0x1.fffffffffffff9p-1022", "4.450147717014403e-308", nil},  // rounded up

      {"0x1.00000000000000p-1022", "2.2250738585072014e-308", nil}, // 0x0010000000000000
      {"0x0.fffffffffffff0p-1022", "2.225073858507201e-308", nil},  // 0x000fffffffffffff
      {"0x0.ffffffffffffe0p-1022", "2.2250738585072004e-308", nil}, // 0x000ffffffffffffe
      {"0x0.ffffffffffffe7p-1022", "2.2250738585072004e-308", nil}, // rounded down
      {"0x1.ffffffffffffe8p-1023", "2.225073858507201e-308", nil},  // rounded up
      {"0x1.ffffffffffffe9p-1023", "2.225073858507201e-308", nil},  // rounded up

      {"0x0.00000003fffff0p-1022", "2.072261e-317", nil},  // 0x00000000003fffff
      {"0x0.00000003456780p-1022", "1.694649e-317", nil},  // 0x0000000000345678
      {"0x0.00000003456787p-1022", "1.694649e-317", nil},  // rounded down
      {"0x0.00000003456788p-1022", "1.694649e-317", nil},  // rounded down (half to even)
      {"0x0.00000003456790p-1022", "1.6946496e-317", nil}, // 0x0000000000345679
      {"0x0.00000003456789p-1022", "1.6946496e-317", nil}, // rounded up

      {"0x0.0000000345678800000000000000000000000001p-1022", "1.6946496e-317", nil}, // rounded up

      {"0x0.000000000000f0p-1022", "7.4e-323", nil}, // 0x000000000000000f
      {"0x0.00000000000060p-1022", "3e-323", nil},   // 0x0000000000000006
      {"0x0.00000000000058p-1022", "3e-323", nil},   // rounded up
      {"0x0.00000000000057p-1022", "2.5e-323", nil}, // rounded down
      {"0x0.00000000000050p-1022", "2.5e-323", nil}, // 0x0000000000000005

      {"0x0.00000000000010p-1022", "5e-324", nil},  // 0x0000000000000001
      {"0x0.000000000000081p-1022", "5e-324", nil}, // rounded up
      {"0x0.00000000000008p-1022", "0", nil},       // rounded down
      {"0x0.00000000000007fp-1022", "0", nil},      // rounded down

      // try to overflow exponent
      {"1e-4294967296", "0", nil},
      {"1e+4294967296", "+Inf", error::range},
      {"1e-18446744073709551616", "0", nil},
      {"1e+18446744073709551616", "+Inf", error::range},
      {"0x1p-4294967296", "0", nil},
      {"0x1p+4294967296", "+Inf", error::range},
      {"0x1p-18446744073709551616", "0", nil},
      {"0x1p+18446744073709551616", "+Inf", error::range},

      // Parse errors
      {"1e", "0", error::syntax},
      {"1e-", "0", error::syntax},
      {".e-1", "0", error::syntax},
      {"1\x00.2"s, "0", error::syntax},
      {"0x", "0", error::syntax},
      {"0x.", "0", error::syntax},
      {"0x1", "0", error::syntax},
      {"0x.1", "0", error::syntax},
      {"0x1p", "0", error::syntax},
      {"0x.1p", "0", error::syntax},
      {"0x1p+", "0", error::syntax},
      {"0x.1p+", "0", error::syntax},
      {"0x1p-", "0", error::syntax},
      {"0x.1p-", "0", error::syntax},
      {"0x1p+2", "4", nil},
      {"0x.1p+2", "0.25", nil},
      {"0x1p-2", "0.25", nil},
      {"0x.1p-2", "0.015625", nil},

      // https://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
      {"2.2250738585072012e-308", "2.2250738585072014e-308", nil},
      // https://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
      {"2.2250738585072011e-308", "2.225073858507201e-308", nil},

      // A very large number (initially wrongly parsed by the fast algorithm).
      {"4.630813248087435e+307", "4.630813248087435e+307", nil},

      // A different kind of very large number.
      {"22.222222222222222", "22.22222222222222", nil},
      {"2." + std::string(4000, '2') + "e+1", "22.22222222222222", nil},
      {"0x1.1111111111111p222", "7.18931911124017e+66", nil},
      {"0x2.2222222222222p221", "7.18931911124017e+66", nil},
      {"0x2." + std::string(4000, '2') + "p221", "7.18931911124017e+66", nil},

      // Exactly halfway between 1 and math.Nextafter(1, 2).
      // Round to even (down).
      {"1.00000000000000011102230246251565404236316680908203125", "1", nil},
      {"0x1.00000000000008p0", "1", nil},
      // Slightly lower; still round down.
      {"1.00000000000000011102230246251565404236316680908203124", "1", nil},
      {"0x1.00000000000007Fp0", "1", nil},
      // Slightly higher; round up.
      {"1.00000000000000011102230246251565404236316680908203126", "1.0000000000000002", nil},
      {"0x1.000000000000081p0", "1.0000000000000002", nil},
      {"0x1.00000000000009p0", "1.0000000000000002", nil},
      // Slightly higher, but you have to read all the way to the end.
      {"1.00000000000000011102230246251565404236316680908203125" + std::string(10000, '0') + "1", "1.0000000000000002", nil},
      {"0x1.00000000000008" + std::string(10000, '0') + "1p0", "1.0000000000000002", nil},

      // Halfway between x := math.Nextafter(1, 2) and math.Nextafter(x, 2)
      // Round to even (up).
      {"1.00000000000000033306690738754696212708950042724609375", "1.0000000000000004", nil},
      {"0x1.00000000000018p0", "1.0000000000000004", nil},

      // Halfway between 1090544144181609278303144771584 and 1090544144181609419040633126912
      // (15497564393479157p+46, should round to even 15497564393479156p+46, issue 36657)
      {"1090544144181609348671888949248", "1.0905441441816093e+30", nil},
      // slightly above, rounds up
      {"1090544144181609348835077142190", "1.0905441441816094e+30", nil},

      // Apostrophes.
      {"1'23.50'0'0e+1'2", "1.235e+14", nil},
      {"-'123.5e+12", "0", error::syntax},
      {"+'123.5e+12", "0", error::syntax},
      {"'123.5e+12", "0", error::syntax},
      {"1''23.5e+12", "0", error::syntax},
      {"123'.5e+12", "0", error::syntax},
      {"123.'5e+12", "0", error::syntax},
      {"123.5'e+12", "0", error::syntax},
      {"123.5''0e+12", "0", error::syntax},
      {"123.5e'+12", "0", error::syntax},
      {"123.5e+'12", "0", error::syntax},
      {"123.5e'-12", "0", error::syntax},
      {"123.5e-'12", "0", error::syntax},
      {"123.5e+1''2", "0", error::syntax},
      {"123.5e+12'", "0", error::syntax},

      {"0x1'2.3'4'5p+1'2", "74565", nil},
      {"0x'1'2.3'4'5p+1'2", "0", error::syntax},
      {"-'0x12.345p+12", "0", error::syntax},
      {"+'0x12.345p+12", "0", error::syntax},
      {"'0x12.345p+12", "0", error::syntax},
      {"0x''12.345p+12", "0", error::syntax},
      {"0x1''2.345p+12", "0", error::syntax},
      {"0x12'.345p+12", "0", error::syntax},
      {"0x12.'345p+12", "0", error::syntax},
      {"0x12.3''45p+12", "0", error::syntax},
      {"0x12.345'p+12", "0", error::syntax},
      {"0x12.345p'+12", "0", error::syntax},
      {"0x12.345p+'12", "0", error::syntax},
      {"0x12.345p'-12", "0", error::syntax},
      {"0x12.345p-'12", "0", error::syntax},
      {"0x12.345p+1''2", "0", error::syntax},
      {"0x12.345p+12'", "0", error::syntax},

      {"1e100x", "0", error::syntax},
      {"1e1000x", "0", error::syntax},
    };
    for (auto test : test_cases) {
      auto in = std::get<0>(test);
      CAPTURE(in);
      auto [out, err] = parse<double>(std::get<0>(test));
      auto outs = format(out, 'g');
      CHECK(outs == std::get<1>(test));
      CHECK(err == std::get<2>(test));
    }
  }
  SECTION("float") {
    auto test_cases = std::vector<std::tuple<std::string, std::string_view, std::error_code>>{
      // Hex
      {"0x1p-100", "7.888609e-31", nil},
      {"0x1p100", "1.2676506e+30", nil},

      // Exactly halfway between 1 and the next float32.
      // Round to even (down).
      {"1.000000059604644775390625", "1", nil},
      {"0x1.000001p0", "1", nil},
      // Slightly lower.
      {"1.000000059604644775390624", "1", nil},
      {"0x1.0000008p0", "1", nil},
      {"0x1.000000fp0", "1", nil},
      // Slightly higher.
      {"1.000000059604644775390626", "1.0000001", nil},
      {"0x1.000002p0", "1.0000001", nil},
      {"0x1.0000018p0", "1.0000001", nil},
      {"0x1.0000011p0", "1.0000001", nil},
      // Slightly higher, but you have to read all the way to the end.
      {"1.000000059604644775390625" + std::string(10000, '0') + "1", "1.0000001", nil},
      {"0x1.000001" + std::string(10000, '0') + "1p0", "1.0000001", nil},

      // largest float32: (1<<128) * (1 - 2^-24)
      {"340282346638528859811704183484516925440", "3.4028235e+38", nil},
      {"-340282346638528859811704183484516925440", "-3.4028235e+38", nil},
      {"0x.ffffffp128", "3.4028235e+38", nil},
      {"-340282346638528859811704183484516925440", "-3.4028235e+38", nil},
      {"-0x.ffffffp128", "-3.4028235e+38", nil},
      // next float32 - too large
      {"3.4028236e38", "+Inf", error::range},
      {"-3.4028236e38", "-Inf", error::range},
      {"0x1.0p128", "+Inf", error::range},
      {"-0x1.0p128", "-Inf", error::range},
      // the border is 3.40282356779...e+38
      // borderline - okay
      {"3.402823567e38", "3.4028235e+38", nil},
      {"-3.402823567e38", "-3.4028235e+38", nil},
      {"0x.ffffff7fp128", "3.4028235e+38", nil},
      {"-0x.ffffff7fp128", "-3.4028235e+38", nil},
      // borderline - too large
      {"3.4028235678e38", "+Inf", error::range},
      {"-3.4028235678e38", "-Inf", error::range},
      {"0x.ffffff8p128", "+Inf", error::range},
      {"-0x.ffffff8p128", "-Inf", error::range},

      // Denormals: less than 2^-126
      {"1e-38", "1e-38", nil},
      {"1e-39", "1e-39", nil},
      {"1e-40", "1e-40", nil},
      {"1e-41", "1e-41", nil},
      {"1e-42", "1e-42", nil},
      {"1e-43", "1e-43", nil},
      {"1e-44", "1e-44", nil},
      {"6e-45", "6e-45", nil}, // 4p-149 = 5.6e-45
      {"5e-45", "6e-45", nil},

      // Smallest denormal
      {"1e-45", "1e-45", nil}, // 1p-149 = 1.4e-45
      {"2e-45", "1e-45", nil},
      {"3e-45", "3e-45", nil},

      // Near denormals and denormals.
      {"0x0.89aBcDp-125", "1.2643093e-38", nil},  // 0x0089abcd
      {"0x0.8000000p-125", "1.1754944e-38", nil}, // 0x00800000
      {"0x0.1234560p-125", "1.671814e-39", nil},  // 0x00123456
      {"0x0.1234567p-125", "1.671814e-39", nil},  // rounded down
      {"0x0.1234568p-125", "1.671814e-39", nil},  // rounded down
      {"0x0.1234569p-125", "1.671815e-39", nil},  // rounded up
      {"0x0.1234570p-125", "1.671815e-39", nil},  // 0x00123457
      {"0x0.0000010p-125", "1e-45", nil},         // 0x00000001
      {"0x0.00000081p-125", "1e-45", nil},        // rounded up
      {"0x0.0000008p-125", "0", nil},             // rounded down
      {"0x0.0000007p-125", "0", nil},             // rounded down

      // 2^92 = 8388608p+69 = 4951760157141521099596496896 (4.9517602e27)
      // is an exact power of two that needs 8 decimal digits to be correctly
      // parsed back.
      // The float32 before is 16777215p+68 = 4.95175986e+27
      // The halfway is 4.951760009. A bad algorithm that thinks the previous
      // float32 is 8388607p+69 will shorten incorrectly to 4.95176e+27.
      {"4951760157141521099596496896", "4.9517602e+27", nil},
    };
    for (auto test : test_cases) {
      auto in = std::get<0>(test);
      CAPTURE(in);
      auto [out, err] = parse<float>(std::get<0>(test));
      auto outs = format(out, 'g');
      CHECK(outs == std::get<1>(test));
      CHECK(err == std::get<2>(test));
    }
  }
}

}  // namespace bongo::strconv
