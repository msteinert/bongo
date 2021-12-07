// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/unicode.h"
#include "bongo/unicode/utf8.h"

using namespace std::string_view_literals;

namespace bongo::unicode::utf8 {

TEST_CASE("UTF-8 constants", "[unicode/utf8]") {
  CHECK(max_rune == unicode::max_rune);
  CHECK(rune_error == unicode::replacement_char);
}

TEST_CASE("UTF-8", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<rune, std::string_view>>{
    {0x0000, "\x00"sv},
    {0x0001, "\x01"},
    {0x007e, "\x7e"},
    {0x007f, "\x7f"},
    {0x0080, "\xc2\x80"},
    {0x0081, "\xc2\x81"},
    {0x00bf, "\xc2\xbf"},
    {0x00c0, "\xc3\x80"},
    {0x00c1, "\xc3\x81"},
    {0x00c8, "\xc3\x88"},
    {0x00d0, "\xc3\x90"},
    {0x00e0, "\xc3\xa0"},
    {0x00f0, "\xc3\xb0"},
    {0x00f8, "\xc3\xb8"},
    {0x00ff, "\xc3\xbf"},
    {0x0100, "\xc4\x80"},
    {0x07ff, "\xdf\xbf"},
    {0x0400, "\xd0\x80"},
    {0x0800, "\xe0\xa0\x80"},
    {0x0801, "\xe0\xa0\x81"},
    {0x1000, "\xe1\x80\x80"},
    {0xd000, "\xed\x80\x80"},
    {0xd7ff, "\xed\x9f\xbf"}, // last code point before surrogate half.
    {0xe000, "\xee\x80\x80"}, // first code point after surrogate half.
    {0xfffe, "\xef\xbf\xbe"},
    {0xffff, "\xef\xbf\xbf"},
    {0x10000, "\xf0\x90\x80\x80"},
    {0x10001, "\xf0\x90\x80\x81"},
    {0x40000, "\xf1\x80\x80\x80"},
    {0x10fffe, "\xf4\x8f\xbf\xbe"},
    {0x10ffff, "\xf4\x8f\xbf\xbf"},
    {0xfffd, "\xef\xbf\xbd"},
  };
  SECTION("Full rune") {
    for (auto [r, s] : test_cases) {
      CAPTURE(r, s);
      CHECK(full_rune(s.begin(), s.end()) == true);
      s = s.substr(0, s.size()-1);
      CHECK(full_rune(s.begin(), s.end()) == false);
    }
    for (auto s : std::vector<std::string_view>{"\xc0", "\xc1"}) {
      CAPTURE(s);
      CHECK(full_rune(s.begin(), s.end()) == true);
    }
  }
  SECTION("Encode") {
    for (auto [r, s] : test_cases) {
      CAPTURE(s);
      std::string out;
      encode(r, std::back_inserter(out));
      CHECK(out == s);
    }
  }
  SECTION("Decode") {
    for (auto [r, s] : test_cases) {
      CAPTURE(r, s);
      CHECK(decode(s.begin(), s.end()) == std::make_pair(r, s.size()));

      // Try with trailing byte
      auto extra = std::string{s.begin(), s.end()} + '\0';
      CHECK(decode(extra.begin(), extra.end()) == std::make_pair(r, s.size()));

      // Missing bytes fail
      size_t exp = 1;
      if (exp >= s.size()) {
        exp = 0;
      }
      CHECK(
          decode(s.begin(), std::prev(s.end()))
          ==
          std::make_pair(rune_error, exp));

      // Bad sequences fail
      auto bad = std::string{s.begin(), s.end()};
      if (s.size() == 1) {
        bad[0] = 0x80;
      } else {
        bad[bad.size()-1] = 0x7f;
      }
      CHECK(
          decode(bad.begin(), bad.end())
          ==
          std::make_pair(rune_error, 1ul));
    }
  }
  SECTION("Sequencing") {
    auto test_strings = std::vector<std::string>{
      "",
      "abcd",
      "☺☻☹",
      "日a本b語ç日ð本Ê語þ日¥本¼語i日©",
      "日a本b語ç日ð本Ê語þ日¥本¼語i日©日a本b語ç日ð本Ê語þ日¥本¼語i日©日a本b語ç日ð本Ê語þ日¥本¼語i日©",
      "\x80\x80\x80\x80",
    };
    for (auto const& ts : test_strings) {
      for (auto [r, view] : test_cases) {
        CAPTURE(r);
        auto str = std::string{view};
        for (auto s : std::vector<std::string>{ts + str, str + ts, ts + str + ts}) {
          auto index = std::vector<std::tuple<size_t, rune>>{};
          index.reserve(count(s.begin(), s.end()));
          size_t si = 0;
          for (auto [i, r] : range{s}) {
            CHECK(si == i);
            index.push_back({i, r});
            auto [r1, size] = decode(std::next(s.begin(), i), s.end());
            CHECK(r == r1);
            si += size;
          }
          si = s.size();
          for (auto it = std::rbegin(index); it < std::rend(index); ++it) {
            auto [r, size] = decode_last(s.begin(), std::next(s.begin(), si));
            si -= size;
            CHECK(si == std::get<0>(*it));
            CHECK(r == std::get<1>(*it));
          }
          CHECK(si == 0);
        }
      }
    }
  }
}

TEST_CASE("Invalid sequence", "[unicode/utf8]") {
  auto test_cases = std::vector<std::string_view>{
    "\xed\xa0\x80\x80", // surrogate min
    "\xed\xbf\xbf\x80", // surrogate max

    // xx
    "\x91\x80\x80\x80",

    // s1
    "\xC2\x7F\x80\x80",
    "\xC2\xC0\x80\x80",
    "\xDF\x7F\x80\x80",
    "\xDF\xC0\x80\x80",

    // s2
    "\xE0\x9F\xBF\x80",
    "\xE0\xA0\x7F\x80",
    "\xE0\xBF\xC0\x80",
    "\xE0\xC0\x80\x80",

    // s3
    "\xE1\x7F\xBF\x80",
    "\xE1\x80\x7F\x80",
    "\xE1\xBF\xC0\x80",
    "\xE1\xC0\x80\x80",

    //s4
    "\xED\x7F\xBF\x80",
    "\xED\x80\x7F\x80",
    "\xED\x9F\xC0\x80",
    "\xED\xA0\x80\x80",

    // s5
    "\xF0\x8F\xBF\xBF",
    "\xF0\x90\x7F\xBF",
    "\xF0\x90\x80\x7F",
    "\xF0\xBF\xBF\xC0",
    "\xF0\xBF\xC0\x80",
    "\xF0\xC0\x80\x80",

    // s6
    "\xF1\x7F\xBF\xBF",
    "\xF1\x80\x7F\xBF",
    "\xF1\x80\x80\x7F",
    "\xF1\xBF\xBF\xC0",
    "\xF1\xBF\xC0\x80",
    "\xF1\xC0\x80\x80",

    // s7
    "\xF4\x7F\xBF\xBF",
    "\xF4\x80\x7F\xBF",
    "\xF4\x80\x80\x7F",
    "\xF4\x8F\xBF\xC0",
    "\xF4\x8F\xC0\x80",
    "\xF4\x90\x80\x80",
  };
  for (auto s : test_cases) {
    auto [r, size] = decode(s.begin(), s.end());
    CAPTURE(size);
    CHECK(r == rune_error);
  }
}

TEST_CASE("Negative runes encode as U+FFFD", "[unicode/utf8]") {
  auto errorbuf = std::vector<char>{};
  encode(rune_error, std::back_inserter(errorbuf));
  auto buf = std::vector<char>{};
  encode(-1, std::back_inserter(buf));
  CHECK(buf == errorbuf);
}

TEST_CASE("Decode surrogate rune", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<rune, std::string_view>>{
    {0xd800, "\xed\xa0\x80"}, // surrogate min decodes to (RuneError, 1)
    {0xdfff, "\xed\xbf\xbf"}, // surrogate max decodes to (RuneError, 1)
  };
  for (auto [r, s] : test_cases) {
    CAPTURE(r, s);
    CHECK(decode(s.begin(), s.end()) == std::make_pair(rune_error, 1ul));
  }
}

TEST_CASE("Count runes", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<std::string_view, size_t>>{
    {"abcd", 4},
    {"☺☻☹", 3},
    {"1,2,3,4", 7},
    {"\xe2\x00"sv, 2},
    {"\xe2\x80", 2},
    {"a\xe2\x80", 3},
  };
  for (auto [s, exp] : test_cases) {
    CAPTURE(s);
    CHECK(count(s.begin(), s.end()) == exp);
  }
}

TEST_CASE("Rune length", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<rune, int>>{
    {0, 1},
    {'e', 1},
    {0x00e9, 2},  // é
    {0x263a, 3},  // ☺
    {rune_error, 3},
    {max_rune, 4},
    {0xd800, -1},
    {0xdfff, -1},
    {max_rune + 1, -1},
    {-1, -1},
  };
  for (auto [r, l] : test_cases) {
    CAPTURE(r);
    CHECK(len(r) == l);
  }
}

TEST_CASE("Valid rune", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<rune, bool>>{
    {0, true},
    {'e', true},
    {0x00e9, true},  // é
    {0x263a, true},  // ☺
    {rune_error, true},
    {max_rune, true},
    {0xd7ff, true},
    {0xd800, false},
    {0xdfff, false},
    {0xe000, true},
    {max_rune + 1, false},
    {-1, false},
  };
  for (auto [r, exp] : test_cases) {
    CAPTURE(r);
    CHECK(valid(r) == exp);
  }
}

TEST_CASE("Valid range", "[unicode/utf8]") {
  auto test_cases = std::vector<std::tuple<std::string_view, bool>>{
    {"", true},
    {"a", true},
    {"abc", true},
    {"Ж", true},
    {"ЖЖ", true},
    {"брэд-ЛГТМ", true},
    {"☺☻☹", true},
    {"aa\xe2", false},
    {"\66\250", false},
    {"\66\250\67", false},
    {"a\uFFFDb", true},
    {"\xF4\x8F\xBF\xBF", true},      // U+10FFFF
    {"\xF4\x90\x80\x80", false},     // U+10FFFF+1; out of range
    {"\xF7\xBF\xBF\xBF", false},     // 0x1FFFFF; out of range
    {"\xFB\xBF\xBF\xBF\xBF", false}, // 0x3FFFFFF; out of range
    {"\xc0\x80", false},             // U+0000 encoded in two bytes: incorrect
    {"\xed\xa0\x80", false},         // U+D800 high surrogate (sic)
    {"\xed\xbf\xbf", false},         // U+DFFF low surrogate (sic)
  };
  for (auto [s, exp] : test_cases) {
    CAPTURE(s);
    CHECK(valid(s.begin(), s.end()) == exp);
  }
}

TEST_CASE("", "[unicode/utf8]") {
  using namespace bongo::unicode::utf8_literals;
  CHECK("界"_rune == 0x754c);
}

}  // namespace bongo::unicode::utf8
