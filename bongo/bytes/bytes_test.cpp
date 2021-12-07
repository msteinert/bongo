// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <compare>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "bongo/bytes.h"

namespace bongo::bytes {

TEST_CASE("Compare", "[bytes]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::string_view,
    std::strong_ordering
  >>{
    {"", "", std::strong_ordering::equal},
    {"a", "", std::strong_ordering::greater},
    {"", "a", std::strong_ordering::less},
    {"abc", "abc", std::strong_ordering::equal},
    {"abd", "abc", std::strong_ordering::greater},
    {"abc", "abd", std::strong_ordering::less},
    {"ab", "abc", std::strong_ordering::less},
    {"abc", "ab", std::strong_ordering::greater},
    {"x", "ab", std::strong_ordering::greater},
    {"ab", "x", std::strong_ordering::less},
    {"x", "a", std::strong_ordering::greater},
    {"b", "x", std::strong_ordering::less},
    {"abcdefgh", "abcdefgh", std::strong_ordering::equal},
    {"abcdefghi", "abcdefghi", std::strong_ordering::equal},
    {"abcdefghi", "abcdefghj", std::strong_ordering::less},
    {"abcdefghj", "abcdefghi", std::strong_ordering::greater},
  };
  for (auto [a, b, exp] : test_cases) {
    CAPTURE(a, b);
    CHECK(compare(to_bytes(a), to_bytes(b)) == exp);
  }
}

TEST_CASE("Compare identical", "[bytes]") {
  auto b = std::string_view{"Hello Gophers!"};
  CHECK(compare(to_bytes(b), to_bytes(b)) == std::strong_ordering::equal);
  CHECK(compare(to_bytes(b), to_bytes(b).subspan(0, 1)) == std::strong_ordering::greater);
}

}  // namespace bongo::bytes
