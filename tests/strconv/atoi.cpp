// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Parse integer", "[strconv]") {
  SECTION("uint64") {
    auto test_cases = std::vector<std::tuple<
      std::string_view,
      std::pair<uint64_t, std::error_code>
    >>{
      {"", {0, bongo::strconv::error::syntax}},
      {"0", {0, std::error_code{}}},
      {"1", {1, std::error_code{}}},
      {"12345", {12345, std::error_code{}}},
      {"012345", {12345, std::error_code{}}},
      {"12345x", {0, bongo::strconv::error::syntax}},
      {"98765432100", {98765432100, std::error_code{}}},
      {"18446744073709551615", {18446744073709551615ull, std::error_code{}}},
      {"18446744073709551616", {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"18446744073709551620", {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"1'2'3'4'5", {0, bongo::strconv::error::syntax}},
      {"'12345", {0, bongo::strconv::error::syntax}},
      {"1''2345", {0, bongo::strconv::error::syntax}},
      {"12345'", {0, bongo::strconv::error::syntax}},
      {"-0", {0, bongo::strconv::error::syntax}},
      {"-1", {0, bongo::strconv::error::syntax}},
      {"+1", {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, exp] : test_cases) {
      auto out = bongo::strconv::parse<uint64_t>(in, 10);
      CAPTURE(in);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
  SECTION("Base uint64") {
    auto test_cases = std::vector<std::tuple<
      std::string_view,
      int,
      std::pair<uint64_t, std::error_code>
    >>{
      {"", 0, {0, bongo::strconv::error::syntax}},
      {"0", 0, {0, std::error_code{}}},
      {"0x", 0, {0, bongo::strconv::error::syntax}},
      {"0X", 0, {0, bongo::strconv::error::syntax}},
      {"1", 0, {1, std::error_code{}}},
      {"12345", 0, {12345, std::error_code{}}},
      {"012345", 0, {012345, std::error_code{}}},
      {"0x12345", 0, {0x12345, std::error_code{}}},
      {"0X12345", 0, {0x12345, std::error_code{}}},
      {"12345x", 0, {0, bongo::strconv::error::syntax}},
      {"0xabcdefg123", 0, {0, bongo::strconv::error::syntax}},
      {"123456789abc", 0, {0, bongo::strconv::error::syntax}},
      {"98765432100", 0, {98765432100, std::error_code{}}},
      {"18446744073709551615", 0, {std::numeric_limits<uint64_t>::max(), std::error_code{}}},
      {"18446744073709551616", 0, {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"18446744073709551620", 0, {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"0xFFFFFFFFFFFFFFFF", 0, {0xFFFFFFFFFFFFFFFF, std::error_code{}}},
      {"0x10000000000000000", 0, {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"01777777777777777777777", 0, {01777777777777777777777, std::error_code{}}},
      {"01777777777777777777778", 0, {0, bongo::strconv::error::syntax}},
      {"02000000000000000000000", 0, {std::numeric_limits<uint64_t>::max(), bongo::strconv::error::range}},
      {"0200000000000000000000", 0, {0200000000000000000000, std::error_code{}}},
      {"0b", 0, {0, bongo::strconv::error::syntax}},
      {"0B", 0, {0, bongo::strconv::error::syntax}},
      {"0b101", 0, {5, std::error_code{}}},
      {"0B101", 0, {5, std::error_code{}}},
      {"0o", 0, {0, bongo::strconv::error::syntax}},
      {"0O", 0, {0, bongo::strconv::error::syntax}},
      {"0o377", 0, {255, std::error_code{}}},
      {"0O377", 0, {255, std::error_code{}}},

      {"1'2'3'4'5", 0, {12345, std::error_code{}}}, // base 0 => 10
      {"'12345", 0, {0, bongo::strconv::error::syntax}},
      {"1''2345", 0, {0, bongo::strconv::error::syntax}},
      {"12345'", 0, {0, bongo::strconv::error::syntax}},

      {"1'2'3'4'5", 10, {0, bongo::strconv::error::syntax}}, // base 10
      {"'12345", 10, {0, bongo::strconv::error::syntax}},
      {"1''2345", 10, {0, bongo::strconv::error::syntax}},
      {"12345'", 10, {0, bongo::strconv::error::syntax}},

      {"0x1'2'3'4'5", 0, {0x12345, std::error_code{}}}, // base 0 => 16
      {"0x'1'2'3'4'5", 0, {0, bongo::strconv::error::syntax}}, // base 0 => 16
      {"'0x12345", 0, {0, bongo::strconv::error::syntax}},
      {"0x''12345", 0, {0, bongo::strconv::error::syntax}},
      {"0x1''2345", 0, {0, bongo::strconv::error::syntax}},
      {"0x1234''5", 0, {0, bongo::strconv::error::syntax}},
      {"0x12345'", 0, {0, bongo::strconv::error::syntax}},

      {"1'2'3'4'5", 16, {0, bongo::strconv::error::syntax}}, // base 16
      {"'12345", 16, {0, bongo::strconv::error::syntax}},
      {"1''2345", 16, {0, bongo::strconv::error::syntax}},
      {"1234''5", 16, {0, bongo::strconv::error::syntax}},
      {"12345'", 16, {0, bongo::strconv::error::syntax}},

      {"0'1'2'3'4'5", 0, {012345, std::error_code{}}}, // base 0 => 8 (0377)
      {"'012345", 0, {0, bongo::strconv::error::syntax}},
      {"0''12345", 0, {0, bongo::strconv::error::syntax}},
      {"01234''5", 0, {0, bongo::strconv::error::syntax}},
      {"012345'", 0, {0, bongo::strconv::error::syntax}},

      {"0o1'2'3'4'5", 0, {012345, std::error_code{}}}, // base 0 => 8 (0o377)
      {"0o'1'2'3'4'5", 0, {0, bongo::strconv::error::syntax}}, // base 0 => 8 (0o377)
      {"'0o12345", 0, {0, bongo::strconv::error::syntax}},
      {"0o''12345", 0, {0, bongo::strconv::error::syntax}},
      {"0o1234''5", 0, {0, bongo::strconv::error::syntax}},
      {"0o12345'", 0, {0, bongo::strconv::error::syntax}},

      {"0'1'2'3'4'5", 8, {0, bongo::strconv::error::syntax}}, // base 8
      {"'012345", 8, {0, bongo::strconv::error::syntax}},
      {"0''12345", 8, {0, bongo::strconv::error::syntax}},
      {"01234''5", 8, {0, bongo::strconv::error::syntax}},
      {"012345'", 8, {0, bongo::strconv::error::syntax}},

      {"0b1'0'1", 0, {5, std::error_code{}}}, // base 0 => 2 (0b101)
      {"0b'1'0'1", 0, {0, bongo::strconv::error::syntax}}, // base 0 => 2 (0b101)
      {"'0b101", 0, {0, bongo::strconv::error::syntax}},
      {"0b''101", 0, {0, bongo::strconv::error::syntax}},
      {"0b1''01", 0, {0, bongo::strconv::error::syntax}},
      {"0b10''1", 0, {0, bongo::strconv::error::syntax}},
      {"0b101'", 0, {0, bongo::strconv::error::syntax}},

      {"1'0'1", 2, {0, bongo::strconv::error::syntax}}, // base 2
      {"'101", 2, {0, bongo::strconv::error::syntax}},
      {"1'01", 2, {0, bongo::strconv::error::syntax}},
      {"10'1", 2, {0, bongo::strconv::error::syntax}},
      {"101'", 2, {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, base, exp] : test_cases) {
      auto out = bongo::strconv::parse<uint64_t>(in, base);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
  SECTION("int64") {
    auto test_cases = std::vector<std::tuple<
      std::string_view,
      std::pair<int64_t, std::error_code>
    >>{
      {"", {0, bongo::strconv::error::syntax}},
      {"0", {0, std::error_code{}}},
      {"-0", {0, std::error_code{}}},
      {"+0", {0, std::error_code{}}},
      {"1", {1, std::error_code{}}},
      {"-1", {-1, std::error_code{}}},
      {"+1", {1, std::error_code{}}},
      {"12345", {12345, std::error_code{}}},
      {"-12345", {-12345, std::error_code{}}},
      {"012345", {12345, std::error_code{}}},
      {"-012345", {-12345, std::error_code{}}},
      {"98765432100", {98765432100, std::error_code{}}},
      {"-98765432100", {-98765432100, std::error_code{}}},
      {"9223372036854775807", {9223372036854775807ull, std::error_code{}}},
      {"-9223372036854775807", {-9223372036854775807ull, std::error_code{}}},
      {"9223372036854775808", {std::numeric_limits<int64_t>::max(), bongo::strconv::error::range}},
      {"-9223372036854775808", {-9223372036854775808ull, std::error_code{}}},
      {"9223372036854775809", {std::numeric_limits<int64_t>::max(), bongo::strconv::error::range}},
      {"-9223372036854775809", {std::numeric_limits<int64_t>::min(), bongo::strconv::error::range}},
      {"-1'2'3'4'5", {0, bongo::strconv::error::syntax}}, // base=10 so no underscores allowed
      {"-'12345", {0, bongo::strconv::error::syntax}},
      {"'12345", {0, bongo::strconv::error::syntax}},
      {"1''2345", {0, bongo::strconv::error::syntax}},
      {"12345'", {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, exp] : test_cases) {
      auto out = bongo::strconv::parse<int64_t>(in, 10);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
  SECTION("Base int64") {
      auto test_cases = std::vector<std::tuple<
      std::string_view,
      int,
      std::pair<int64_t, std::error_code>
    >>{
      {"", 0, {0, bongo::strconv::error::syntax}},
      {"0", 0, {0, std::error_code{}}},
      {"-0", 0, {0, std::error_code{}}},
      {"1", 0, {1, std::error_code{}}},
      {"-1", 0, {-1, std::error_code{}}},
      {"12345", 0, {12345, std::error_code{}}},
      {"-12345", 0, {-12345, std::error_code{}}},
      {"012345", 0, {012345, std::error_code{}}},
      {"-012345", 0, {-012345, std::error_code{}}},
      {"0x12345", 0, {0x12345, std::error_code{}}},
      {"-0X12345", 0, {-0x12345, std::error_code{}}},
      {"12345x", 0, {0, bongo::strconv::error::syntax}},
      {"-12345x", 0, {0, bongo::strconv::error::syntax}},
      {"98765432100", 0, {98765432100, std::error_code{}}},
      {"-98765432100", 0, {-98765432100, std::error_code{}}},
      {"9223372036854775807", 0, {std::numeric_limits<int64_t>::max(), std::error_code{}}},
      {"-9223372036854775807", 0, {std::numeric_limits<int64_t>::min()+1, std::error_code{}}},
      {"9223372036854775808", 0, {std::numeric_limits<int64_t>::max(), bongo::strconv::error::range}},
      {"-9223372036854775808", 0, {std::numeric_limits<int64_t>::min(), std::error_code{}}},
      {"9223372036854775809", 0, {std::numeric_limits<int64_t>::max(), bongo::strconv::error::range}},
      {"-9223372036854775809", 0, {std::numeric_limits<int64_t>::min(), bongo::strconv::error::range}},

      // other bases
      {"g", 17, {16, std::error_code{}}},
      {"10", 25, {25, std::error_code{}}},
      {"holycow", 35, {(((((17*35+24)*35+21)*35+34)*35+12)*35+24)*35ll + 32ll, std::error_code{}}},
      {"holycow", 36, {(((((17*36+24)*36+21)*36+34)*36+12)*36+24)*36ll + 32ll, std::error_code{}}},

      // base 2
      {"0", 2, {0, std::error_code{}}},
      {"-1", 2, {-1, std::error_code{}}},
      {"1010", 2, {10, std::error_code{}}},
      {"1000000000000000", 2, {1 << 15, std::error_code{}}},
      {"111111111111111111111111111111111111111111111111111111111111111", 2, {std::numeric_limits<int64_t>::max(), std::error_code{}}},
      {"1000000000000000000000000000000000000000000000000000000000000000", 2, {std::numeric_limits<int64_t>::max(), bongo::strconv::error::range}},
      {"-1000000000000000000000000000000000000000000000000000000000000000", 2, {std::numeric_limits<int64_t>::min(), std::error_code{}}},
      {"-1000000000000000000000000000000000000000000000000000000000000001", 2, {std::numeric_limits<int64_t>::min(), bongo::strconv::error::range}},

      // base 8
      {"-10", 8, {-8, std::error_code{}}},
      {"57635436545", 8, {057635436545, std::error_code{}}},
      {"100000000", 8, {1 << 24, std::error_code{}}},

      // base 16
      {"10", 16, {16, std::error_code{}}},
      {"-123456789abcdef", 16, {-0x123456789abcdef, std::error_code{}}},
      {"7fffffffffffffff", 16, {std::numeric_limits<int64_t>::max(), std::error_code{}}},

      // underscores
      {"-0x1'2'3'4'5", 0, {-0x12345, std::error_code{}}},
      {"-0x'1'2'3'4'5", 0, {0, bongo::strconv::error::syntax}},
      {"0x1'2'3'4'5", 0, {0x12345, std::error_code{}}},
      {"0x'1'2'3'4'5", 0, {0, bongo::strconv::error::syntax}},
      {"-'0x12345", 0, {0, bongo::strconv::error::syntax}},
      {"'-0x12345", 0, {0, bongo::strconv::error::syntax}},
      {"'0x12345", 0, {0, bongo::strconv::error::syntax}},
      {"0x''12345", 0, {0, bongo::strconv::error::syntax}},
      {"0x1''2345", 0, {0, bongo::strconv::error::syntax}},
      {"0x1234''5", 0, {0, bongo::strconv::error::syntax}},
      {"0x12345'", 0, {0, bongo::strconv::error::syntax}},

      {"-0'1'2'3'4'5", 0, {-012345, std::error_code{}}}, // octal
      {"0'1'2'3'4'5", 0, {012345, std::error_code{}}},   // octal
      {"-'012345", 0, {0, bongo::strconv::error::syntax}},
      {"'-012345", 0, {0, bongo::strconv::error::syntax}},
      {"'012345", 0, {0, bongo::strconv::error::syntax}},
      {"0''12345", 0, {0, bongo::strconv::error::syntax}},
      {"01234''5", 0, {0, bongo::strconv::error::syntax}},
      {"012345'", 0, {0, bongo::strconv::error::syntax}},

      {"+0xf", 0, {0xf, std::error_code{}}},
      {"-0xf", 0, {-0xf, std::error_code{}}},
      {"0x+f", 0, {0, bongo::strconv::error::syntax}},
      {"0x-f", 0, {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, base, exp] : test_cases) {
      auto out = bongo::strconv::parse<int64_t>(in, base);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
  SECTION("uint32") {
      auto test_cases = std::vector<std::tuple<
      std::string_view,
      std::pair<uint32_t, std::error_code>
    >>{
      {"", {0, bongo::strconv::error::syntax}},
      {"0", {0, std::error_code{}}},
      {"1", {1, std::error_code{}}},
      {"12345", {12345, std::error_code{}}},
      {"012345", {12345, std::error_code{}}},
      {"12345x", {0, bongo::strconv::error::syntax}},
      {"987654321", {987654321, std::error_code{}}},
      {"4294967295", {std::numeric_limits<uint32_t>::max(), std::error_code{}}},
      {"4294967296", {std::numeric_limits<uint32_t>::max(), bongo::strconv::error::range}},
      {"1'2'3'4'5", {0, bongo::strconv::error::syntax}}, // base=10 so no underscores allowed
      {"'12345", {0, bongo::strconv::error::syntax}},
      {"'12345", {0, bongo::strconv::error::syntax}},
      {"1''2345", {0, bongo::strconv::error::syntax}},
      {"12345'", {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, exp] : test_cases) {
      auto out = bongo::strconv::parse<uint32_t>(in, 10);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
  SECTION("int32") {
      auto test_cases = std::vector<std::tuple<
      std::string_view,
      std::pair<int32_t, std::error_code>
    >>{
      {"", {0, bongo::strconv::error::syntax}},
      {"0", {0, std::error_code{}}},
      {"-0", {0, std::error_code{}}},
      {"1", {1, std::error_code{}}},
      {"-1", {-1, std::error_code{}}},
      {"12345", {12345, std::error_code{}}},
      {"-12345", {-12345, std::error_code{}}},
      {"012345", {12345, std::error_code{}}},
      {"-012345", {-12345, std::error_code{}}},
      {"12345x", {0, bongo::strconv::error::syntax}},
      {"-12345x", {0, bongo::strconv::error::syntax}},
      {"987654321", {987654321, std::error_code{}}},
      {"-987654321", {-987654321, std::error_code{}}},
      {"2147483647", {std::numeric_limits<int32_t>::max(), std::error_code{}}},
      {"-2147483647", {std::numeric_limits<int32_t>::min()+1, std::error_code{}}},
      {"2147483648", {std::numeric_limits<int32_t>::max(), bongo::strconv::error::range}},
      {"-2147483648", {std::numeric_limits<int32_t>::min(), std::error_code{}}},
      {"2147483649", {std::numeric_limits<int32_t>::max(), bongo::strconv::error::range}},
      {"-2147483649", {std::numeric_limits<int32_t>::min(), bongo::strconv::error::range}},
      {"-1'2'3'4'5", {0, bongo::strconv::error::syntax}}, // base=10 so no underscores allowed
      {"-'12345", {0, bongo::strconv::error::syntax}},
      {"'12345", {0, bongo::strconv::error::syntax}},
      {"1''2345", {0, bongo::strconv::error::syntax}},
      {"12345'", {0, bongo::strconv::error::syntax}},
    };
    for (auto [in, exp] : test_cases) {
      auto out = bongo::strconv::parse<int32_t>(in, 10);
      CHECK(std::get<0>(out) == std::get<0>(exp));
      CHECK(std::get<1>(out) == std::get<1>(exp));
    }
  }
}
