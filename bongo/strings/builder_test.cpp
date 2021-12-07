// Copyright The Go Authors.

#include <functional>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bytes.h"
#include "bongo/fmt.h"
#include "bongo/strings.h"
#include "bongo/unicode/utf8.h"

namespace bongo::strings {

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace unicode::utf8_literals;

void check(builder& b, std::string_view want) {
  auto got = b.str();
  CHECK(got == want);
  CHECK(b.size() == got.size());
  CHECK(b.capacity() >= got.size());
}

TEST_CASE("String builder: 1", "[strings]") {
  auto b = builder{};
  check(b, "");
  auto [n, err] = b.write_string("hello");
  CHECK(err == nil);
  CHECK(n == 5);
  check(b, "hello");
  err = b.write_byte(' ');
  CHECK(err == nil);
  check(b, "hello ");
  std::tie(n, err) = b.write_string("world");
  CHECK(err == nil);
  CHECK(n == 5);
  check(b, "hello world");
}

TEST_CASE("String builder: 2", "[strings]") {
  auto b = builder{};
  b.write_string("alpha");
  check(b, "alpha");
  auto s1 = std::string{b.str()};
  b.write_string("beta");
  check(b, "alphabeta");
  auto s2 = std::string{b.str()};
  b.write_string("gamma");
  check(b, "alphabetagamma");
  auto s3 = std::string{b.str()};

  CHECK(s1 == "alpha");
  CHECK(s2 == "alphabeta");
  CHECK(s3 == "alphabetagamma");
}

TEST_CASE("String builder: reset", "[strings]") {
  auto b = builder{};
  check(b, "");
  b.write_string("aaa");
  auto s = std::string{b.str()};
  check(b, "aaa");
  b.reset();
  check(b, "");

  b.write_string("bbb");
  check(b, "bbb");
  CHECK(s == "aaa");
}

TEST_CASE("String builder: grow", "[strings]") {
  for (auto grow_len : std::vector<size_t>{0, 100, 1000, 10000, 100000}) {
    auto arr = std::array<uint8_t, 1>{'a'};
    auto p = bytes::repeat(arr, grow_len);
    auto b = builder{};
    b.grow(grow_len);
    CHECK(b.capacity() >= grow_len);
    b.write(p);
    CHECK(b.str() == bytes::to_string(p));
  }
}

TEST_CASE("String builder: write 2", "[strings]") {
  struct test_case {
    std::string name;
    std::function<std::pair<int, std::error_code>(builder&)> fn;
    int n;
    std::string want;
  };
  auto s0 = "hello 世界"s;
  auto test_cases = std::vector<test_case>{
    test_case{
      "write",
      [&](builder& b) { return b.write(bytes::to_bytes(s0)); },
      static_cast<int>(s0.size()),
      s0
    },
    test_case{
      "write_rune",
      [](builder& b) { return b.write_rune('a'); },
      1,
      "a"
    },
    test_case{
      "write_rune wide",
      [](builder& b) { return b.write_rune("世"_rune); },
      3,
      "世"
    },
    test_case{
      "write_string",
      [&](builder& b) { return b.write_string(s0); },
      static_cast<int>(s0.size()),
      s0
    },
  };
  for (auto& tt : test_cases) {
    INFO(tt.name);
    auto b = builder{};

    auto [n, err] = tt.fn(b);
    CHECK(err == nil);
    CHECK(n == tt.n);
    check(b, tt.want);

    std::tie(n, err) = tt.fn(b);
    CHECK(err == nil);
    CHECK(n == tt.n);
    check(b, tt.want+tt.want);
  }
}

TEST_CASE("String builder: write byte", "[strings]") {
  auto b = builder{};
  CHECK(b.write_byte('a') == nil);
  CHECK(b.write_byte(0) == nil);
  check(b, "a\x00"sv);
}

TEST_CASE("String builder: write invalid rune", "[strings]") {
  for (auto r : std::vector<rune>{-1, unicode::utf8::max_rune+1}) {
    auto b = builder{};
    b.write_rune(r);
    check(b, "\ufffd"sv);
  }
}

}  // namespace bongo::strings
