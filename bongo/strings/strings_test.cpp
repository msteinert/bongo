// Copyright The Go Authors.

#include <limits>
#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strings.h"

using namespace std::string_view_literals;

namespace bongo::strings {

constexpr static std::string_view abcd = "abcd";
constexpr static std::string_view faces = "☺☻☹";
constexpr static std::string_view commas = "1,2,3,4";
constexpr static std::string_view dots = "1....2....3....4";

TEST_CASE("Test index", "[strings]") {
  auto npos = std::string_view::npos;
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    std::string_view::size_type
  >>{
    {"", "", 0},
    {"", "a", npos},
    {"", "foo", npos},
    {"fo", "foo", npos},
    {"foo", "foo", 0},
    {"oofofoofooo", "f", 2},
    {"oofofoofooo", "foo", 4},
    {"barfoobarfoo", "foo", 3},
    {"foo", "", 0},
    {"foo", "o", 1},
    {"abcABCabc", "A", 3},
    // cases with one byte strings - test special case in Index()
    {"", "a", npos},
    {"x", "a", npos},
    {"x", "x", 0},
    {"abc", "a", 0},
    {"abc", "b", 1},
    {"abc", "c", 2},
    {"abc", "x", npos},
    // test special cases in Index() for short strings
    {"", "ab", npos},
    {"bc", "ab", npos},
    {"ab", "ab", 0},
    {"xab", "ab", 1},
    {"xab"sv.substr(0, 2), "ab", npos},
    {"", "abc", npos},
    {"xbc", "abc", npos},
    {"abc", "abc", 0},
    {"xabc", "abc", 1},
    {"xabc"sv.substr(0, 3), "abc", npos},
    {"xabxc", "abc", npos},
    {"", "abcd", npos},
    {"xbcd", "abcd", npos},
    {"abcd", "abcd", 0},
    {"xabcd", "abcd", 1},
    {"xyabcd"sv.substr(0, 5), "abcd", npos},
    {"xbcqq", "abcqq", npos},
    {"abcqq", "abcqq", 0},
    {"xabcqq", "abcqq", 1},
    {"xyabcqq"sv.substr(0, 6), "abcqq", npos},
    {"xabxcqq", "abcqq", npos},
    {"xabcqxq", "abcqq", npos},
    {"", "01234567", npos},
    {"32145678", "01234567", npos},
    {"01234567", "01234567", 0},
    {"x01234567", "01234567", 1},
    {"x0123456x01234567", "01234567", 9},
    {"xx01234567"sv.substr(0, 9), "01234567", npos},
    {"", "0123456789", npos},
    {"3214567844", "0123456789", npos},
    {"0123456789", "0123456789", 0},
    {"x0123456789", "0123456789", 1},
    {"x012345678x0123456789", "0123456789", 11},
    {"xyz0123456789"sv.substr(0, 12), "0123456789", npos},
    {"x01234567x89", "0123456789", npos},
    {"", "0123456789012345", npos},
    {"3214567889012345", "0123456789012345", npos},
    {"0123456789012345", "0123456789012345", 0},
    {"x0123456789012345", "0123456789012345", 1},
    {"x012345678901234x0123456789012345", "0123456789012345", 17},
    {"", "01234567890123456789", npos},
    {"32145678890123456789", "01234567890123456789", npos},
    {"01234567890123456789", "01234567890123456789", 0},
    {"x01234567890123456789", "01234567890123456789", 1},
    {"x0123456789012345678x01234567890123456789", "01234567890123456789", 21},
    {"xyz01234567890123456789"sv.substr(0, 22), "01234567890123456789", npos},
    {"", "0123456789012345678901234567890", npos},
    {"321456788901234567890123456789012345678911", "0123456789012345678901234567890", npos},
    {"0123456789012345678901234567890", "0123456789012345678901234567890", 0},
    {"x0123456789012345678901234567890", "0123456789012345678901234567890", 1},
    {"x012345678901234567890123456789x0123456789012345678901234567890", "0123456789012345678901234567890", 32},
    {"xyz0123456789012345678901234567890"sv.substr(0, 33), "0123456789012345678901234567890", npos},
    {"", "01234567890123456789012345678901", npos},
    {"32145678890123456789012345678901234567890211", "01234567890123456789012345678901", npos},
    {"01234567890123456789012345678901", "01234567890123456789012345678901", 0},
    {"x01234567890123456789012345678901", "01234567890123456789012345678901", 1},
    {"x0123456789012345678901234567890x01234567890123456789012345678901", "01234567890123456789012345678901", 33},
    {"xyz01234567890123456789012345678901"sv.substr(0, 34), "01234567890123456789012345678901", npos},
    {"xxxxxx012345678901234567890123456789012345678901234567890123456789012", "012345678901234567890123456789012345678901234567890123456789012", 6},
    {"", "0123456789012345678901234567890123456789", npos},
    {"xx012345678901234567890123456789012345678901234567890123456789012", "0123456789012345678901234567890123456789", 2},
    {"xx012345678901234567890123456789012345678901234567890123456789012"sv.substr(0, 41), "0123456789012345678901234567890123456789", npos},
    {"xx012345678901234567890123456789012345678901234567890123456789012", "0123456789012345678901234567890123456xxx", npos},
    {"xx0123456789012345678901234567890123456789012345678901234567890120123456789012345678901234567890123456xxx", "0123456789012345678901234567890123456xxx", 65},
    // test fallback to Rabin-Karp.
    {"oxoxoxoxoxoxoxoxoxoxoxoy", "oy", 22},
    {"oxoxoxoxoxoxoxoxoxoxoxox", "oy", npos},
  };
  for (auto [s, substr, exp] : test_cases) {
    CAPTURE(s, substr);
    CHECK(index(s, substr) == exp);
  }
}

TEST_CASE("Test last_index", "[strings]") {
  auto npos = std::string_view::npos;
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    std::string_view::size_type
  >>{
    {"", "", 0},
    {"", "a", npos},
    {"", "foo", npos},
    {"fo", "foo", npos},
    {"foo", "foo", 0},
    {"foo", "f", 0},
    {"oofofoofooo", "f", 7},
    {"oofofoofooo", "foo", 7},
    {"barfoobarfoo", "foo", 9},
    {"foo", "", 3},
    {"foo", "o", 2},
    {"abcABCabc", "A", 3},
    {"abcABCabc", "a", 6},
  };
  for (auto [s, substr, exp] : test_cases) {
    CAPTURE(s, substr);
    CHECK(last_index(s, substr) == exp);
  }
}

TEST_CASE("Test count", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    int
  >>{
    {"", "", 1},
    {"", "notempty", 0},
    {"notempty", "", 9},
    {"smaller", "not smaller", 0},
    {"12345678987654321", "6", 2},
    {"611161116", "6", 3},
    {"notequal", "NotEqual", 0},
    {"equal", "equal", 1},
    {"abc1231231123q", "123", 3},
    {"11111", "11", 2},
  };
  for (auto [s, substr, exp] : test_cases) {
    CAPTURE(s, substr);
    CHECK(count(s, substr) == exp);
  }
}

TEST_CASE("Test contains", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    bool
  >>{
    {"abc", "bc", true},
    {"abc", "bcd", false},
    {"abc", "", true},
    {"", "a", false},
    // 2-byte needle
    {"xxxxxx", "01", false},
    {"01xxxx", "01", true},
    {"xx01xx", "01", true},
    {"xxxx01", "01", true},
    {"01xxxxx"sv.substr(1), "01", false},
    {"xxxxx01"sv.substr(0, 6), "01", false},
    // 3-byte needle
    {"xxxxxxx", "012", false},
    {"012xxxx", "012", true},
    {"xx012xx", "012", true},
    {"xxxx012", "012", true},
    {"012xxxxx"sv.substr(1), "012", false},
    {"xxxxx012"sv.substr(0, 7), "012", false},
    // 4-byte needle
    {"xxxxxxxx", "0123", false},
    {"0123xxxx", "0123", true},
    {"xx0123xx", "0123", true},
    {"xxxx0123", "0123", true},
    {"0123xxxxx"sv.substr(1), "0123", false},
    {"xxxxx0123"sv.substr(0, 8), "0123", false},
    // 5-7-byte needle
    {"xxxxxxxxx", "01234", false},
    {"01234xxxx", "01234", true},
    {"xx01234xx", "01234", true},
    {"xxxx01234", "01234", true},
    {"01234xxxxx"sv.substr(1), "01234", false},
    {"xxxxx01234"sv.substr(0, 9), "01234", false},
    // 8-byte needle
    {"xxxxxxxxxxxx", "01234567", false},
    {"01234567xxxx", "01234567", true},
    {"xx01234567xx", "01234567", true},
    {"xxxx01234567", "01234567", true},
    {"01234567xxxxx"sv.substr(1), "01234567", false},
    {"xxxxx01234567"sv.substr(0, 12), "01234567", false},
    // 9-15-byte needle
    {"xxxxxxxxxxxxx", "012345678", false},
    {"012345678xxxx", "012345678", true},
    {"xx012345678xx", "012345678", true},
    {"xxxx012345678", "012345678", true},
    {"012345678xxxxx"sv.substr(1), "012345678", false},
    {"xxxxx012345678"sv.substr(0, 13), "012345678", false},
    // 16-byte needle
    {"xxxxxxxxxxxxxxxxxxxx", "0123456789ABCDEF", false},
    {"0123456789ABCDEFxxxx", "0123456789ABCDEF", true},
    {"xx0123456789ABCDEFxx", "0123456789ABCDEF", true},
    {"xxxx0123456789ABCDEF", "0123456789ABCDEF", true},
    {"0123456789ABCDEFxxxxx"sv.substr(1), "0123456789ABCDEF", false},
    {"xxxxx0123456789ABCDEF"sv.substr(0, 20), "0123456789ABCDEF", false},
    // 17-31-byte needle
    {"xxxxxxxxxxxxxxxxxxxxx", "0123456789ABCDEFG", false},
    {"0123456789ABCDEFGxxxx", "0123456789ABCDEFG", true},
    {"xx0123456789ABCDEFGxx", "0123456789ABCDEFG", true},
    {"xxxx0123456789ABCDEFG", "0123456789ABCDEFG", true},
    {"0123456789ABCDEFGxxxxx"sv.substr(1), "0123456789ABCDEFG", false},
    {"xxxxx0123456789ABCDEFG"sv.substr(0, 21), "0123456789ABCDEFG", false},
    // partial match cases
    {"xx01x", "012", false},                             // 3
    {"xx0123x", "01234", false},                         // 5-7
    {"xx01234567x", "012345678", false},                 // 9-15
    {"xx0123456789ABCDEFx", "0123456789ABCDEFG", false}, // 17-31
  };
  for (auto [s, substr, exp] : test_cases) {
    CHECK(contains(s, substr) == exp);
  }
}

TEST_CASE("Test repeat", "[strings]") {
  auto test_cases = std::vector<std::tuple<std::string_view, std::string_view, int>>{
    {"", "", 0},
    {"", "", 1},
    {"", "", 2},
    {"-", "", 0},
    {"-", "----------", 10},
    {"abc ", "abc abc abc ", 3},
  };
  for (auto const& [in, out, count] : test_cases) {
    CHECK(repeat(in, count) == out);
  }
}

TEST_CASE("Test split", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    int,
    std::vector<std::string_view>
  >>{
    {"", "", -1, {}},
    {abcd, "", 2, {"a", "bcd"}},
    {abcd, "", 4, {"a", "b", "c", "d"}},
    {abcd, "", -1, {"a", "b", "c", "d"}},
    {faces, "", -1, {"☺", "☻", "☹"}},
    {faces, "", 3, {"☺", "☻", "☹"}},
    {faces, "", 17, {"☺", "☻", "☹"}},
    {"☺�☹", "", -1, {"☺", "�", "☹"}},
    {abcd, "a", 0, {}},
    {abcd, "a", -1, {"", "bcd"}},
    {abcd, "z", -1, {"abcd"}},
    {commas, ",", -1, {"1", "2", "3", "4"}},
    {dots, "...", -1, {"1", ".2", ".3", ".4"}},
    {faces, "☹", -1, {"☺☻", ""}},
    {faces, "~", -1, {faces}},
    {"1 2 3 4", " ", 3, {"1", "2", "3 4"}},
    {"1 2", " ", 3, {"1", "2"}},
    {"", "T", std::numeric_limits<int>::max() / 4, {""}},
    {"\xff-\xff", "", -1, {"\xff", "-", "\xff"}},
    {"\xff-\xff", "-", -1, {"\xff", "\xff"}},
  };
  for (auto [s, sep, n, exp] : test_cases) {
    auto v = split(s, sep, n);
    CHECK(v == exp);
    if (n == 0) {
      continue;
    }
    CHECK(join(v, sep) == s);
    if (n < 0) {
      CHECK(split(s, sep) == v);
    }
  }
}

TEST_CASE("Test split_after", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    int,
    std::vector<std::string_view>
  >>{
    {abcd, "a", -1, {"a", "bcd"}},
    {abcd, "z", -1, {"abcd"}},
    {abcd, "", -1, {"a", "b", "c", "d"}},
    {commas, ",", -1, {"1,", "2,", "3,", "4"}},
    {dots, "...", -1, {"1...", ".2...", ".3...", ".4"}},
    {faces, "☹", -1, {"☺☻☹", ""}},
    {faces, "~", -1, {faces}},
    {faces, "", -1, {"☺", "☻", "☹"}},
    {"1 2 3 4", " ", 3, {"1 ", "2 ", "3 4"}},
    {"1 2 3", " ", 3, {"1 ", "2 ", "3"}},
    {"1 2", " ", 3, {"1 ", "2"}},
    {"123", "", 2, {"1", "23"}},
    {"123", "", 17, {"1", "2", "3"}},
  };
  for (auto [s, sep, n, exp] : test_cases) {
    auto v = split_after(s, sep, n);
    CHECK(v == exp);
    CHECK(join(v, "") == s);
    if (n < 0) {
      CHECK(split_after(s, sep) == v);
    }
  }
}

TEST_CASE("Test replace", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    std::string_view,
    int,
    std::string_view
  >>{
    {"hello", "l", "L", 0, "hello"},
    {"hello", "l", "L", -1, "heLLo"},
    {"hello", "x", "X", -1, "hello"},
    {"", "x", "X", -1, ""},
    {"radar", "r", "<r>", -1, "<r>ada<r>"},
    {"", "", "<>", -1, "<>"},
    {"banana", "a", "<>", -1, "b<>n<>n<>"},
    {"banana", "a", "<>", 1, "b<>nana"},
    {"banana", "a", "<>", 1000, "b<>n<>n<>"},
    {"banana", "an", "<>", -1, "b<><>a"},
    {"banana", "ana", "<>", -1, "b<>na"},
    {"banana", "", "<>", -1, "<>b<>a<>n<>a<>n<>a<>"},
    {"banana", "", "<>", 10, "<>b<>a<>n<>a<>n<>a<>"},
    {"banana", "", "<>", 6, "<>b<>a<>n<>a<>n<>a"},
    {"banana", "", "<>", 5, "<>b<>a<>n<>a<>na"},
    {"banana", "", "<>", 1, "<>banana"},
    {"banana", "a", "a", -1, "banana"},
    {"banana", "a", "a", 1, "banana"},
    {"☺☻☹", "", "<>", -1, "<>☺<>☻<>☹<>"},
  };
  for (auto [s, old_s, new_s, n, exp] : test_cases) {
    CAPTURE(s, old_s, new_s, n);
    CHECK(replace(s, old_s, new_s, n) == exp);
    if (n == -1) {
      CHECK(replace(s, old_s, new_s) == exp);
    }
  }
}

TEST_CASE("Test cut", "[strings]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    std::tuple<std::string_view, std::string_view, bool>
  >>{
    {"abc", "b", {"a", "c", true}},
    {"abc", "a", {"", "bc", true}},
    {"abc", "c", {"ab", "", true}},
    {"abc", "abc", {"", "", true}},
    {"abc", "", {"", "abc", true}},
    {"abc", "d", {"abc", "", false}},
    {"", "d", {"", "", false}},
    {"", "", {"", "", true}},
  };
  for (auto [s, sep, exp] : test_cases) {
    CHECK(cut(s, sep) == exp);
  }
}

}  // namespace bongo::strings
