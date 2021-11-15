// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/unicode.h"

TEST_CASE("UTF-16 constants", "[unicode][utf16]") {
  CHECK(bongo::unicode::utf16::max_rune == bongo::unicode::max_rune);
  CHECK(bongo::unicode::utf16::replacement_char == bongo::unicode::replacement_char);
}

TEST_CASE("UTF-16 encode", "[unicode][utf16]") {
  auto test_cases = std::vector<std::tuple<std::vector<bongo::rune>, std::vector<uint16_t>>>{
    {{1, 2, 3, 4}, {1, 2, 3, 4}},
    {
      {0xffff, 0x10000, 0x10001, 0x12345, 0x10ffff},
      {0xffff, 0xd800, 0xdc00, 0xd800, 0xdc01, 0xd808, 0xdf45, 0xdbff, 0xdfff}
    },
    {
      {'a', 'b', 0xd7ff, 0xd800, 0xdfff, 0xe000, 0x110000, -1},
      {'a', 'b', 0xd7ff, 0xfffd, 0xfffd, 0xe000, 0xfffd, 0xfffd}
    },
  };
  SECTION("Encode range") {
    for (auto [in, exp] : test_cases) {
      std::vector<uint16_t> out;
      bongo::unicode::utf16::encode(in.begin(), in.end(), std::back_inserter(out));
      CHECK(out == exp);
    }
  }
  SECTION("Encode rune") {
    for (auto [in, exp] : test_cases) {
      size_t j = 0;
      for (auto r : in) {
        auto [r1, r2] = bongo::unicode::utf16::encode(r);
        if (r <0x10000 || r > bongo::unicode::max_rune) {
          CHECK(j < exp.size());
          CHECK(r1 == bongo::unicode::replacement_char);
          CHECK(r2 == bongo::unicode::replacement_char);
          ++j;
        } else {
          CHECK(j+1 < exp.size());
          CHECK(r1 == static_cast<bongo::rune>(exp[j]));
          CHECK(r2 == static_cast<bongo::rune>(exp[j+1]));
          CHECK(bongo::unicode::utf16::decode(r1, r2) == r);
          j += 2;
        }
      }
      CHECK(j == exp.size());
    }
  }
}

TEST_CASE("UTF-16 decode range", "[unicode][utf16]") {
  auto test_cases = std::vector<std::tuple<std::vector<uint16_t>, std::vector<bongo::rune>>> {
    {{1, 2, 3, 4}, {1, 2, 3, 4}},
    {
      {0xffff, 0xd800, 0xdc00, 0xd800, 0xdc01, 0xd808, 0xdf45, 0xdbff, 0xdfff},
      {0xffff, 0x10000, 0x10001, 0x12345, 0x10ffff}
    },
    {{0xd800, 'a'}, {0xfffd, 'a'}},
    {{0xdfff}, {0xfffd}},
  };
  for (auto [in, exp] : test_cases) {
    std::vector<bongo::rune> out;
    bongo::unicode::utf16::decode(in.begin(), in.end(), std::back_inserter(out));
    REQUIRE(out == exp);
  }
}

TEST_CASE("UTF-16 decode rune", "[unicode][utf16]") {
  auto test_cases = std::vector<std::tuple<bongo::rune, bongo::rune, bongo::rune>>{
    {0xd800, 0xdc00, 0x10000},
    {0xd800, 0xdc01, 0x10001},
    {0xd808, 0xdf45, 0x12345},
    {0xdbff, 0xdfff, 0x10ffff},
    {0xd800, 'a', 0xfffd}, // illegal, replacement rune substituted
  };
  for (auto [r1, r2, exp] : test_cases) {
    CHECK(bongo::unicode::utf16::decode(r1, r2) == exp);
  }
}

TEST_CASE("UTF-16 surrogates", "[unicode][utf16]") {
  auto test_cases = std::vector<std::tuple<bongo::rune, bool>>{
    {0x007a,   false}, // LATIN SMALL LETTER Z
    {0x6c34,   false}, // CJK UNIFIED IDEOGRAPH-6C34 (water)
    {0xfeff,   false}, // Byte Order Mark
    {0x10000,  false}, // LINEAR B SYLLABLE B008 A (first non-BMP code point)
    {0x1d11e,  false}, // MUSICAL SYMBOL G CLEF
    {0x10fffd, false}, // PRIVATE USE CHARACTER-10FFFD (last Unicode code point)

    {0xd7ff,   false}, // surr1-1
    {0xd800,   true},  // surr1
    {0xdc00,   true},  // surr2
    {0xe000,   false}, // surr3
    {0xdfff,   true},  // surr3-1
  };
  for (auto [r, exp] : test_cases) {
    CAPTURE(r);
    CHECK(bongo::unicode::utf16::is_surrogate(r) == exp);
  }
}
