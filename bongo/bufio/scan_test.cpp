// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <optional>
#include <string_view>
#include <span>
#include <system_error>
#include <tuple>
#include <vector>
#include <utility>

#include "bongo/bufio.h"
#include "bongo/strings.h"
#include "bongo/testing.h"
#include "bongo/unicode/utf8.h"

namespace bongo::bufio {

namespace utf8 = unicode::utf8;
using namespace std::string_view_literals;

static constexpr long small_max_token_size = 256;

TEST_CASE("Bufio scanner", "[bufio]") {
  auto test_cases = std::vector<std::string_view>{
    "",
    "a",
    "¼",
    "☹",
    "\x81",   // UTF-8 error
    "\ufffd", // correctly encoded RuneError
    "abcdefgh",
    "abc def\n\t\tgh    ",
    "abc¼☹\u0081\ufffd日本語\u0082abc",
  };
  SECTION("Scan bytes") {
    for (auto test : test_cases) {
      auto buf = strings::reader{test};
      auto s = scanner{buf, &scan_bytes};
      size_t i;
      for (i = 0; s.scan(); ++i) {
        auto b = s.bytes();
        CHECK(b.size() == 1);
        CHECK(b[0] == static_cast<uint8_t>(test[i]));
      }
      CHECK(i == test.size());
      CHECK(s.err() == nil);
    }
  }
  SECTION("Scan runes") {
    for (auto test : test_cases) {
      auto buf = strings::reader{test};
      auto s = scanner{buf, &scan_runes};
      size_t rune_count = 0;
      for (auto [i, expect] : utf8::range{test}) {
        if (!s.scan()) {
          break;
        }
        ++rune_count;
        auto [got, _] = utf8::decode(s.bytes());
        CAPTURE(i);
        CHECK(got == expect);
      }
      CHECK(s.scan() == false);
      CHECK(rune_count == utf8::count(test));
      CHECK(s.err() == nil);
    }
  }
}

TEST_CASE("Scan words", "[bufio]") {
  auto test_cases = std::vector<std::pair<std::string_view, std::vector<std::string_view>>>{
    {"", {}},
    {" ", {}},
    {"\n", {}},
    {"a", {"a"}},
    {" a ", {"a"}},
    {"abc def", {"abc", "def"}},
    {" abc def ", {"abc", "def"}},
    {" abc\tdef\nghi\rjkl\fmno\vpqr\u0085stu\u00a0\n", {"abc", "def", "ghi", "jkl", "mno", "pqr", "stu"}},
  };
  for (auto [test, exp] : test_cases) {
    auto buf = strings::reader{test};
    auto s = scanner{buf, &scan_words};
    size_t word_count = 0;
    for (; word_count < exp.size(); ++word_count) {
      if (!s.scan()) {
        break;
      }
      CHECK(s.text() == exp[word_count]);
    }
    CHECK(s.scan() == false);
    CHECK(word_count == exp.size());
    CHECK(s.err() == nil);
  }
}

template <typename T> requires io::ReaderFunc<T>
class slow_reader {
  T* r_;
  long max_;

 public:
  slow_reader(T& r, long max)
      : r_{std::addressof(r)}
      , max_{max} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    if (p.size() > static_cast<size_t>(max_)) {
      p = p.subspan(0, max_);
    }
    return io::read(*r_, p);
  }
};

auto gen_line(bytes::buffer& buf, long line_num, long n, bool add_newline) -> void {
  buf.reset();
  auto do_cr = (line_num%5 == 0);
  if (do_cr) {
    --n;
  }
  for (long i = 0; i < n-1; ++i) {
    uint8_t c = static_cast<uint8_t>('a') + static_cast<uint8_t>(line_num+i);
    if (c == '\n' || c == '\r') {
      c = 'N';
    }
    buf.write_byte(c);
  }
  if (add_newline) {
    if (do_cr) {
      buf.write_byte('\r');
    }
    buf.write_byte('\n');
  }
}

TEST_CASE("Scan long lines", "[bufio]") {
  auto tmp = bytes::buffer{};
  auto buf = bytes::buffer{};
  for (long line_num = 0, j = 0; line_num < 2*small_max_token_size; ++line_num) {
    gen_line(tmp, line_num, j, true);
    if (j < small_max_token_size) {
      ++j;
    } else {
      --j;
    }
    buf.write(tmp.bytes());
  }
  auto r = slow_reader{buf, 1};
  auto s = scanner{r, &scan_lines};
  s.buffer({}, small_max_token_size);
  s.max_token_size(small_max_token_size);
  for (long line_num = 0, j = 0; s.scan(); ++line_num) {
    gen_line(tmp, line_num, j, false);
    if (j < small_max_token_size) {
      ++j;
    } else {
      --j;
    }
    CAPTURE(line_num);
    REQUIRE(s.text() == tmp.str());
  }
  CHECK(s.err() == nil);
}

TEST_CASE("Scan line too long", "[bufio]") {
  auto tmp = bytes::buffer{};
  auto buf = bytes::buffer{};
  for (long line_num = 0, j = 0; line_num < 2*small_max_token_size; ++line_num, ++j) {
    gen_line(tmp, line_num, j, true);
    buf.write(tmp.bytes());
  }
  auto r = slow_reader(buf, 3);
  auto s = scanner(r, &scan_lines);
  s.max_token_size(small_max_token_size);
  for (long line_num = 0, j = 0; s.scan(); ++line_num) {
    gen_line(tmp, line_num, j, false);
    if (j < small_max_token_size) {
      ++j;
    } else {
      --j;
    }
    CAPTURE(line_num, s.text(), tmp.str());
    REQUIRE(bytes::equal(s.bytes(), tmp.bytes()) == true);
  }
  CHECK(s.err() == error::token_too_long);
}

auto test_no_newline(std::string_view text, std::span<std::string_view> lines) -> void {
  auto buf = strings::reader{text};
  auto slo = slow_reader{buf, 7};
  auto s = scanner{slo, &scan_lines};
  for (long line_num = 0; s.scan(); ++line_num) {
    CHECK(s.text() == lines[line_num]);
  }
  CHECK(s.err() == nil);
}

TEST_CASE("Scan line no newline", "[bufio]") {
   auto text = "abcdefghijklmn\nopqrstuvwxyz";
   auto lines = std::vector<std::string_view>{
     "abcdefghijklmn",
     "opqrstuvwxyz",
   };
   test_no_newline(text, lines);
}

TEST_CASE("Scan line return but no newline", "[bufio]") {
   auto text = "abcdefghijklmn\nopqrstuvwxyz\r";
   auto lines = std::vector<std::string_view>{
     "abcdefghijklmn",
     "opqrstuvwxyz",
   };
   test_no_newline(text, lines);
}

TEST_CASE("Scan line empty final line", "[bufio]") {
   auto text = "\nabcdefghijklmn\n\nopqrstuvwxyz\n\n";
   auto lines = std::vector<std::string_view>{
     "",
     "abcdefghijklmn",
     "",
     "opqrstuvwxyz",
     "",
   };
   test_no_newline(text, lines);
}

TEST_CASE("Scan line empty final line with CR", "[bufio]") {
   auto text = "abcdefghijklmn\nopqrstuvwxyz\n\r";
   auto lines = std::vector<std::string_view>{
     "abcdefghijklmn",
     "opqrstuvwxyz",
     "",
   };
   test_no_newline(text, lines);
}

TEST_CASE("Split error", "[bufio]") {
  auto num_splits = 0;
  constexpr auto ok_count = 7;
  constexpr auto text = "abcdefghijklmnopqrstuvwxyz";
  auto r = strings::reader{text};
  auto s = scanner{r, [&num_splits, &ok_count](std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
    if (at_eof) {
      throw std::runtime_error{"didn't get enough data"};
    }
    if (num_splits >= ok_count) {
      return {0, nil, testing::error::test_error};
    }
    ++num_splits;
    return {1, data.subspan(0, 1), nil};
  }};
  long i = 0;
  for (; s.scan(); ++i) {
    CHECK(s.bytes().size() == 1);
    CHECK(s.bytes()[0] == text[i]);
  }
  CHECK(i == ok_count);
  CHECK(s.err() == testing::error::test_error);
}

TEST_CASE("Error at EOF", "[bufio]") {
  auto r = strings::reader{"1 2 33"};
  auto s = scanner{r, [](std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
    auto [advance, token, err] = scan_words(data, at_eof);
    if (token && token->size() > 1) {
      err = testing::error::test_error;
    }
    return {advance, token, err};
  }};
  for (; s.scan();) {}
  CHECK(s.err() == testing::error::test_error);
}

struct always_error {
  auto read(std::span<uint8_t>) -> std::pair<long, std::error_code> {
    return {0, io::error::unexpected_eof};
  }
};

TEST_CASE("Non-EOF with empty read", "[bufio]") {
  auto r = always_error{};
  auto s = scanner{r, &scan_lines};
  CHECK(s.scan() == false);
  CHECK(s.err() == io::error::unexpected_eof);
}

struct endless_zeroes {
  auto read(std::span<uint8_t>) -> std::pair<long, std::error_code> {
    return {0, nil};
  }
};

TEST_CASE("Bad reader", "[bufio]") {
  auto r = endless_zeroes{};
  auto s = scanner{r, &scan_lines};
  CHECK(s.scan() == false);
  CHECK(s.err() == io::error::no_progress);
}

TEST_CASE("Scan word excessive whitespace", "[bufio]") {
  constexpr auto word = "ipsum";
  auto text = strings::repeat(" ", 4*small_max_token_size) + word;
  auto r = strings::reader{text};
  auto s = scanner{r, &scan_words};
  CHECK(s.scan() == true);
  CHECK(s.text() == word);
}

auto comma_split(std::span<uint8_t const> data, bool) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  for (size_t i = 0; i < data.size(); ++i) {
    if (data[i] == ',') {
      return {i + 1, data.subspan(0, i), nil};
    }
  }
  return {0, data, error::final_token};
}

auto test_empty_tokens(std::string_view text, std::span<std::string_view> values) -> void {
  auto r = strings::reader{text};
  auto s = scanner{r, &comma_split};
  size_t i = 0;
  for (; s.scan(); ++i) {
    REQUIRE(i < values.size());
    CHECK(s.text() == values[i]);
  }
  CHECK(i == values.size());
  CHECK(s.err() == nil);
}

TEST_CASE("Empty tokens", "[bufio]") {
  auto text = "1,2,3,";
  std::vector<std::string_view> values = {"1", "2", "3", ""};
  test_empty_tokens(text, values);
}

TEST_CASE("With no empty tokens", "[bufio]") {
  auto text = "1,2,3";
  std::vector<std::string_view> values = {"1", "2", "3"};
  test_empty_tokens(text, values);
}

auto loop_at_eof_split(std::span<uint8_t const> data, bool) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  if (data.size() > 0) {
    return {1, data.subspan(0, 1), nil};
  }
  return {0, data, nil};
}

TEST_CASE("Don't loop forever", "[bufio]") {
  auto r = strings::reader{"abc"};
  auto s = scanner{r, &loop_at_eof_split};
  REQUIRE_THROWS_AS([&s]() {
    for (long count = 0; s.scan(); ++count) {
      REQUIRE(count <= 1000);
    }
  }(), std::system_error);
  CHECK(s.err() == nil);
}

TEST_CASE("Blank lines", "[bufio]") {
  auto text = strings::repeat("\n", 1000);
  auto r = strings::reader{text};
  auto s = scanner{r, &scan_lines};
  for (long count = 0; s.scan(); ++count) {
    REQUIRE(count <= 2000);
  }
  CHECK(s.err() == nil);
}

TEST_CASE("Empty lines OK", "[bufio]") {
  long c = 10000;
  auto text = strings::repeat("\n", 10000);
  auto r = strings::reader{text};
  auto s = scanner{r, [&c](std::span<uint8_t const> data, bool) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
    if (c > 0) {
      --c;
      return {1, data.subspan(0, 1), nil};
    }
    return {0, std::nullopt, nil};
  }};
  for (; s.scan();) {}
  CHECK(s.err() == nil);
  CHECK(c == 0);
}

TEST_CASE("Huge buffer", "[bufio]") {
  auto text = strings::repeat("x", 2*max_scan_token_size);
  auto r = strings::reader{text};
  auto s = scanner(r, &scan_lines);
  auto v = std::vector<uint8_t>{};
  v.resize(100);
  s.buffer(std::move(v), 3*max_scan_token_size);
  for (; s.scan();) {
    CHECK(s.text() == text);
  }
  CHECK(s.err() == nil);
}

struct negative_eof_reader {
  long r;
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    auto c = r;
    if (c > 0) {
      if (static_cast<size_t>(c) > p.size()) {
        c = p.size();
      }
      for (long i = 0; i < c; ++i) {
        p[i] = 'a';
      }
      p[c-1] = '\n';
      r -= c;
      return {c, nil};
    }
    return {-1, io::eof};
  }
};

TEST_CASE("Negative EOF reader", "[bufio]") {
  auto r = negative_eof_reader{10};
  auto s = scanner{r, &scan_lines};
  auto c = 0;
  for (; s.scan();) {
    ++c;
    REQUIRE(c <= 1);
  }
  CHECK(s.err() == error::bad_read_count);
}

struct large_reader {
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    return {p.size() + 1, nil};
  }
};

TEST_CASE("Large reader", "[bufio]") {
  auto r = large_reader{};
  auto s = scanner{r, &scan_lines};
  for (; s.scan();) {}
  CHECK(s.err() == error::bad_read_count);
}

}  // namespace bongo::bufio
