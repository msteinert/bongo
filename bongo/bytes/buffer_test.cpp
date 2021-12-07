// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <algorithm>
#include <random>
#include <string>
#include <string_view>

#include "bongo/bytes.h"
#include "bongo/unicode/utf8.h"

namespace bongo::bytes {

using namespace std::string_view_literals;

std::string init_test_string() {
  constexpr static int n = 10000;
  auto s = std::string{};
  s.reserve(n);
  for (auto i = 0; i < n; ++i) {
    s.push_back('a' + static_cast<char>(i%26));
  }
  return s;
}

static std::string test_string = init_test_string();
static std::vector<uint8_t> test_bytes = std::vector<uint8_t>{test_string.begin(), test_string.end()};

void check(std::string const& name, buffer const& buf, std::string_view s) {
  INFO(name);
  auto bytes = buf.bytes();
  auto str = buf.str();
  CHECK(buf.size() == bytes.size());
  CHECK(buf.size() == str.size());
  CHECK(buf.size() == s.size());
  CHECK(to_string(bytes) == str);
}

void empty(std::string const& name, buffer& buf, std::string_view s, std::vector<uint8_t>& fub) {
  check(name+" (empty 1)", buf, s);
  for (;;) {
    auto [n, err] = buf.read(fub);
    if (n == 0) {
      break;
    }
    INFO(name+" (empty 2)");
    CHECK(err == nil);
    s = s.substr(n);
    check(name+" (empty 3)", buf, s);
  }
  check(name+" (empty 4)", buf, "");
}

std::string fill_string(std::string const& name, buffer& buf, std::string s, int n, std::string_view fus) {
  check(name+" (fill 1)", buf, s);
  for (; n > 0; --n) {
    auto [m, err] = buf.write_string(fus);
    INFO(name+" (fill 2)");
    CHECK(m == static_cast<int64_t>(fus.size()));
    CHECK(err == nil);
    s += fus;
    check(name+" (fill 3)", buf, s);
  }
  return s;
}

std::string fill_bytes(std::string const& name, buffer& buf, std::string s, int n, std::span<uint8_t> fub) {
  check(name+" (fill 1)", buf, s);
  for (; n > 0; --n) {
    auto [m, err] = buf.write(fub);
    INFO(name+" (fill 2)");
    CHECK(m == static_cast<int64_t>(fub.size()));
    CHECK(err == nil);
    s += to_string(fub);
    check(name+" (fill 3)", buf, s);
  }
  return s;
}

TEST_CASE("Byte buffer", "[bytes]") {
  auto buf = buffer{test_bytes};
  check("Byte buffer", buf, test_string);
}

TEST_CASE("String buffer", "[bytes]") {
  auto buf = buffer{test_string};
  check("String buffer", buf, test_string);
}

TEST_CASE("Basic buffer operations", "[bytes]") {
  auto buf = buffer{};
  for (auto i = 0; i < 5; ++i) {
    check("Basic buffer operations (1)", buf, "");

    buf.reset();
    check("Basic buffer operations (2)", buf, "");

    buf.truncate(0);
    check("Basic buffer operations (3)", buf, "");

    auto [n, err] = buf.write(std::span{test_bytes}.subspan(0, 1));
    CHECK(n == 1);
    CHECK(err == nil);
    check("Basic buffer operations (4)", buf, "a");

    buf.write_byte(test_string[1]);
    check("Basic buffer operations (5)", buf, "ab");

    std::tie(n, err) = buf.write(std::span{test_bytes}.subspan(2, 24));
    CHECK(n == 24);
    CHECK(err == nil);
    check("Basic buffer operations (6)", buf, std::string_view{test_string}.substr(0, 26));

    buf.truncate(26);
    check("Basic buffer operations (7)", buf, std::string_view{test_string}.substr(0, 26));

    buf.truncate(20);
    check("Basic buffer operations (8)", buf, std::string_view{test_string}.substr(0, 20));

    auto v = std::vector<uint8_t>{};
    v.resize(5);
    empty("Basic buffer operations (9)", buf, std::string_view{test_string}.substr(0, 20), v);
    v.resize(100);
    empty("Basic buffer operations (10)", buf, "", v);

    buf.write_byte(test_string[1]);
    char c;
    std::tie(c, err) = buf.read_byte();
    CHECK(c == test_string[1]);
    CHECK(err == nil);

    std::tie(c, err) = buf.read_byte();
    CHECK(err == io::eof);
  }
}

TEST_CASE("Buffer large string writes", "[bytes]") {
  auto buf = buffer{};
  auto limit = 30;
  auto v = std::vector<uint8_t>{};
  for (auto i = 3; i < limit; i += 3) {
    auto s = fill_string("Buffer large string writes (1)", buf, "", 5, test_string);
    v.resize(test_string.size()/i);
    empty("Buffer large string writes (2)", buf, s, v);
  }
  check("Buffer large string writes (3)", buf, "");
}

TEST_CASE("Buffer large byte writes", "[bytes]") {
  auto buf = buffer{};
  auto limit = 30;
  auto v = std::vector<uint8_t>{};
  for (auto i = 3; i < limit; i += 3) {
    auto s = fill_bytes("Buffer large byte writes (1)", buf, "", 5, test_bytes);
    v.resize(test_string.size()/i);
    empty("Buffer large byte writes (2)", buf, s, v);
  }
  check("Buffer large byte writes (3)", buf, "");
}

TEST_CASE("Buffer large string reads", "[bytes]") {
  auto buf = buffer{};
  auto v = std::vector<uint8_t>{};
  v.resize(test_string.size());
  for (auto i = 3; i < 30; i += 3) {
    auto s = fill_string(
        "Buffer large string reads (1)",
        buf, "", 5, std::string_view{test_string}.substr(0, test_string.size()/i));
    empty("Buffer large string reads (2)", buf, s, v);
  }
  check("Buffer large string reads (3)", buf, "");
}

TEST_CASE("Buffer large byte reads", "[bytes]") {
  auto buf = buffer{};
  auto v = std::vector<uint8_t>{};
  v.resize(test_string.size());
  for (auto i = 3; i < 30; i += 3) {
    auto s = fill_bytes(
        "Buffer large byte reads (1)",
        buf, "", 5, std::span<uint8_t>{test_bytes}.subspan(0, test_bytes.size()/i));
    empty("Buffer large byte reads (2)", buf, s, v);
  }
  check("Buffer large byte reads (3)", buf, "");
}

TEST_CASE("Buffer mixed reads and writes", "[bytes]") {
  std::mt19937 gen{std::random_device{}()};
  auto r = std::uniform_int_distribution<>{0, static_cast<int>(test_string.size())};
  auto buf = buffer{};
  auto s = std::string{};
  for (auto i = 0; i < 50; ++i) {
    auto wlen = r(gen);
    if (i%2 == 0) {
      s = fill_string(
          "Buffer mixed reads and writes (1)",
          buf, s, 1, std::string_view{test_string}.substr(0, wlen));
    } else {
      s = fill_bytes(
          "Buffer mixed reads and writes (1)",
          buf, s, 1, std::span<uint8_t>(test_bytes).subspan(0, wlen));
    }
    auto fub = std::vector<uint8_t>{};
    fub.resize(r(gen));
    auto [n, _] = buf.read(fub);
    s = s.substr(n);
  }
  auto v = std::vector<uint8_t>{};
  v.resize(buf.size());
  empty("Buffer mixed reads and writes (2)", buf, s, v);
}

TEST_CASE("Buffer read_from", "[bytes]") {
  auto buf = buffer{};
  auto v = std::vector<uint8_t>{};
  v.resize(test_string.size());
  for (auto i = 3; i < 30; i += 3) {
    auto s = fill_bytes("Buffer read_from (1)", buf, "", 5, std::span{test_bytes}.subspan(0, test_bytes.size()/i));
    auto b = buffer{};
    b.read_from(buf);
    empty("Buffer read_from (2)", b, s, v);
  }
}

struct throwing_reader {
  bool should_throw = false;
  std::pair<int, std::error_code> read(std::span<uint8_t>) {
    if (should_throw) {
      throw std::exception{};
    }
    return {0, io::eof};
  }
};

TEST_CASE("Buffer read from throwing reader", "[bytes]") {
  auto buf = buffer{};
  auto r1 = throwing_reader{};
  auto [i, err] = buf.read_from(r1);
  REQUIRE(err == nil);
  REQUIRE(i == 0);
  check("Buffer read from throwing reader (1)", buf, "");

  auto buf2 = buffer{};
  auto r2 = throwing_reader{true};
  REQUIRE_THROWS_AS(buf2.read_from(r2), std::exception);
  check("Buffer read from throwing reader (2)", buf, "");
}

struct negative_reader {
  std::pair<int, std::error_code> read(std::span<uint8_t>) {
    return {-1, nil};
  }
};

TEST_CASE("Buffer read from negative reader", "[bytes]") {
  auto b = buffer{};
  auto r = negative_reader{};
  REQUIRE_THROWS_AS(b.read_from(r), std::exception);
}

TEST_CASE("Buffer write_to", "[bytes]") {
  auto buf = buffer{};
  auto v = std::vector<uint8_t>{};
  v.resize(test_string.size());
  for (auto i = 3; i < 30; i += 3) {
    auto s = fill_bytes("Buffer write_to (1)", buf, "", 5, std::span{test_bytes}.subspan(0, test_bytes.size()/i));
    auto b = buffer{};
    buf.write_to(b);
    empty("Buffer write_to (2)", b, s, v);
  }
}

TEST_CASE("Buffer rune I/O", "[bytes]") {
  auto n_rune = 1000;
  auto v1 = std::vector<uint8_t>(unicode::utf8::utf_max*n_rune);
  auto b = std::span{v1};
  auto buf = buffer{};
  auto n = 0;
  for (auto r = rune{0}; r < n_rune; ++r) {
    auto begin = b.subspan(n).begin();
    auto size = std::distance(begin, unicode::utf8::encode(r, begin));
    auto [n_bytes, err] = buf.write_rune(r);
    REQUIRE(err == nil);
    REQUIRE(n_bytes == static_cast<int>(size));
    n += size;
  }

  b = b.subspan(0, n);
  REQUIRE(equal(buf.bytes(), b));

  auto v2 = std::vector<uint8_t>(unicode::utf8::utf_max);
  auto p = std::span{v2};
  for (auto r = rune{0}; r < n_rune; ++r) {
    auto begin = p.begin();
    auto size = std::distance(begin, unicode::utf8::encode(r, begin));
    auto [nr, n_bytes, err] = buf.read_rune();
    REQUIRE(nr == r);
    REQUIRE(n_bytes == static_cast<int>(size));
    REQUIRE(err == nil);
  }

  buf.reset();
  auto err = buf.unread_rune();
  REQUIRE(err != nil);

  std::tie(std::ignore, std::ignore, err) = buf.read_rune();
  REQUIRE(err != nil);

  err = buf.unread_rune();
  REQUIRE(err != nil);

  buf.write(b);
  for (auto r = rune{0}; r < n_rune; ++r) {
    auto [r1, size, _] = buf.read_rune();
    REQUIRE(buf.unread_rune() == nil);
    auto [r2, n_bytes, err] = buf.read_rune();
    REQUIRE(r1 == r2);
    REQUIRE(r1 == r);
    REQUIRE(n_bytes == static_cast<int>(size));
    REQUIRE(err == nil);
  }
}

TEST_CASE("Buffer write invalid rune", "[bytes]") {
  for (auto r : std::vector<rune>{-1, unicode::utf8::max_rune + 1}) {
    auto buf = buffer{};
    buf.write_rune(r);
    check("Buffer write invalid rune ("+std::to_string(r)+")", buf, "\ufffd");
  }
}

TEST_CASE("Buffer next", "[bytes]") {
  auto b = std::vector<uint8_t>{0, 1, 2, 3, 4};
  auto tmp = std::vector<uint8_t>(5);
  for (auto i = 0; i <= 5; ++i) {
    for (auto j = i; j <= 5; ++j) {
      for (auto k = 0; k <= 6; ++k) {
        auto buf = buffer{std::span{b}.subspan(0, j)};
        auto [n, _] = buf.read(std::span{tmp}.subspan(0, i));
        REQUIRE(n == i);
        auto bb = buf.next(k);
        auto want = k;
        if (want > j-i) {
          want = j - i;
        }
        REQUIRE(static_cast<int>(bb.size()) == want);
        for (auto l = 0; l < static_cast<int>(bb.size()); ++l) {
          REQUIRE(bb[l] == static_cast<uint8_t>(l+i));
        }
      }
    }
  }
}

TEST_CASE("Buffer read", "[bytes]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    uint8_t,
    std::vector<std::string_view>,
    std::error_code
  >>{
    {"", 0, {""}, io::eof},
    {"a\x00"sv, 0, {"a\x00"sv}, nil},
    {"abbbaaaba", 'b', {"ab", "b", "b", "aaab"}, nil},
    {"hello\x01world", 1, {"hello\x01"}, nil},
    {"foo\nbar", 0, {"foo\nbar"}, io::eof},
    {"alpha\nbeta\ngamma\n", '\n', {"alpha\n", "beta\n", "gamma\n"}, nil},
    {"alpha\nbeta\ngamma", '\n', {"alpha\n", "beta\n", "gamma"}, io::eof},
  };
  SECTION("Bytes") {
    for (auto& [data, delim, expected, error] : test_cases) {
      auto buf = buffer{data};
      std::error_code err;
      for (auto exp : expected) {
        std::span<uint8_t> bytes;
        std::tie(bytes, err) = buf.read_bytes(delim);
        CHECK(to_string(bytes) == exp);
        if (err) {
          break;
        }
      }
      CHECK(err == error);
    }
  }
  SECTION("String") {
    for (auto& [data, delim, expected, error] : test_cases) {
      auto buf = buffer{data};
      std::error_code err;
      for (auto exp : expected) {
        std::string_view s;
        std::tie(s, err) = buf.read_string(delim);
        CHECK(s == exp);
        if (err) {
          break;
        }
      }
      CHECK(err == error);
    }
  }
}

TEST_CASE("Buffer read empty at EOF", "[bytes]") {
  auto b = buffer{};
  auto s = std::span<uint8_t>{};
  auto [n, err] = b.read(s);
  CHECK(err == nil);
  CHECK(n == 0);
}

TEST_CASE("Buffer unread byte", "[bytes]") {
  auto b = buffer{};

  // At EOF
  CHECK(b.unread_byte() != nil);
  CHECK(b.read_byte().second != nil);
  CHECK(b.unread_byte() != nil);

  // Not at EOF
  b.write_string("abcdefghijklmnopqrstuvwxyz");

  // After unsuccessful read
  auto [n, err1] = b.read(nil);
  CHECK(n == 0);
  CHECK(err1 == nil);
  CHECK(b.unread_byte() != nil);

  // After successful read
  CHECK(b.read_bytes('m').second == nil);
  CHECK(b.unread_byte() == nil);
  auto [c, err2] = b.read_byte();
  CHECK(c == 'm');
  CHECK(err2 == nil);
}

}  // namespace bongo::bytes
