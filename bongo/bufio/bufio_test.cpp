// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <span>
#include <system_error>
#include <thread>
#include <utility>

#include "bongo/bufio.h"
#include "bongo/bytes.h"
#include "bongo/fmt.h"
#include "bongo/io.h"
#include "bongo/strconv.h"
#include "bongo/strings.h"
#include "bongo/testing.h"
#include "bongo/testing/iotest.h"
#include "bongo/time.h"
#include "bongo/unicode/utf8.h"

namespace bongo::bufio {

namespace iotest = testing::iotest;
namespace utf8 = unicode::utf8;

using namespace std::chrono_literals;
using namespace std::string_view_literals;

template <typename T> requires io::ReaderFunc<T>
class rot13_reader {
  T* r_;

 public:
  rot13_reader(T& r)
      : r_{std::addressof(r)} {}

  auto read(std::span<uint8_t> p) noexcept -> std::pair<long, std::error_code> {
    using io::read;
    auto [n, err] = read(*r_, p);
    for (auto i = 0; i < n; ++i) {
      auto c = p[i] | 0x20;
      if ('a' <= c && c <= 'm') {
        p[i] += 13;
      } else if ('n' <= c && c <= 'z') {
        p[i] -= 13;
      }
    }
    return {n, err};
  }
};

template <typename T> requires io::Reader<T>
class passthru_reader {
  T* r_;
 public:
  passthru_reader(T& r)
      : r_{std::addressof(r)} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code>
  { using io::read; return read(*r_, p); }
};

template <typename T> requires io::Writer<T>
class passthru_writer {
  T* w_;
 public:
  passthru_writer(T& w)
      : w_{std::addressof(w)} {}
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code>
  { using io::write; return write(*w_, p); }
};

template <typename T> requires io::ByteReader<T>
auto read_bytes(T& buf) -> std::string {
  uint8_t b[1000];
  auto nb = 0;
  for (;;) {
    auto [c, err] = buf.read_byte();
    if (err == io::eof) {
      break;
    }
    if (err == nil) {
      b[nb++] = c;
    } else if (err != iotest::error::timeout) {
      throw std::system_error{err};
    }
  }
  return std::string{bytes::to_string(b, nb)};
}

TEST_CASE("Simple bufio reader", "[bufio]") {
  auto data = "hello world";
  auto s = strings::reader{data};
  auto b1 = reader{s};
  CHECK(read_bytes(b1) == "hello world");

  s = strings::reader{data};
  auto r = rot13_reader{s};
  auto b2 = reader{r};
  CHECK(read_bytes(b2) == "uryyb jbeyq");
}

template <typename T>
auto read_lines(T& b) -> std::string {
  std::string s = "";
  for (;;) {
    auto [s1, err] = b.read_string('\n');
    if (err == io::eof) {
      break;
    }
    if (err != nil && err != iotest::error::timeout) {
      throw std::system_error{err};
    }
    s += s1;
  }
  return s;
}

struct reader_wrapper {
  using read_wrapper = std::function<std::pair<long, std::error_code>(std::span<uint8_t>)>;
  using read_byte_wrapper = std::function<std::pair<uint8_t, std::error_code>()>;
  using read_string_wrapper = std::function<std::pair<std::string, std::error_code>(uint8_t)>;
  read_wrapper read_fn_;
  read_byte_wrapper read_byte_fn_;
  read_string_wrapper read_string_fn_;
  reader_wrapper(read_wrapper&& read_fn)
      : read_fn_{std::move(read_fn)} {}
  reader_wrapper(read_wrapper&& read_fn, read_byte_wrapper&& read_byte_fn)
      : read_fn_{std::move(read_fn)}
      , read_byte_fn_{std::move(read_byte_fn)} {}
  reader_wrapper(read_wrapper&& read_fn, read_byte_wrapper&& read_byte_fn, read_string_wrapper&& read_string_fn)
      : read_fn_{std::move(read_fn)}
      , read_byte_fn_{std::move(read_byte_fn)}
      , read_string_fn_{std::move(read_string_fn)} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> { return read_fn_(p); }
  auto read_byte() -> std::pair<uint8_t, std::error_code> { return read_byte_fn_(); }
  auto read_string(uint8_t delim) -> std::pair<std::string, std::error_code> { return read_string_fn_(delim); }
};

template <typename T>
auto reads(T& r, long m) -> std::string {
  uint8_t b[1000];
  auto nb = 0;
  for (;;) {
    auto [n, err] = r.read(std::span{b+nb, static_cast<size_t>(m)});
    nb += n;
    if (err == io::eof) {
      break;
    }
  }
  return std::string{bytes::to_string(b, nb)};
}

struct buffer_reader {
  std::string name;
  std::function<std::string(reader_wrapper&)> fn;
};

static std::vector<buffer_reader> buffer_readers = {
  {"1", [](reader_wrapper& r) { return reads(r, 1); }},
  {"2", [](reader_wrapper& r) { return reads(r, 2); }},
  {"3", [](reader_wrapper& r) { return reads(r, 3); }},
  {"4", [](reader_wrapper& r) { return reads(r, 4); }},
  {"5", [](reader_wrapper& r) { return reads(r, 5); }},
  {"7", [](reader_wrapper& r) { return reads(r, 7); }},
  {"bytes", [](reader_wrapper& r) { return read_bytes(r); }},
  {"lines", [](reader_wrapper& r) { return read_lines(r); }},
};

static constexpr long min_read_buffer_size = 16;

static std::vector<long> buffer_sizes = {
  0, 16, 23, 32, 46, 64, 93, 128, 1024, 4096,
};

template <template <typename> typename T>
auto test_readers() -> void {
  std::array<std::string, 31> texts;
  std::string str = "", all = "";
  for (size_t i = 0; i < texts.size()-1; ++i) {
    texts[i] = str + "\n";
    all += texts[i];
    str += utf8::encode(static_cast<rune>(i%26 + 'a'));
  }
  texts[texts.size()-1] = all;

  for (auto const& text : texts) {
    for (auto const& buf_reader : buffer_readers) {
      for (auto size : buffer_sizes) {
        CAPTURE(size, text);
        auto t = strings::reader{text};
        auto read = T{t};
        auto buf = reader{read, size};
        auto wrap2 = reader_wrapper{
          [&buf](std::span<uint8_t> p) { return buf.read(p); },
          [&buf]() { return buf.read_byte(); },
          [&buf](uint8_t delim) { return buf.read_string(delim); },
        };
        CHECK(buf_reader.fn(wrap2) == text);
      }
    }
  }
}

TEST_CASE("Bufio reader", "[bufio]") {
  test_readers<passthru_reader>();
  test_readers<iotest::one_byte_reader>();
  test_readers<iotest::half_reader>();
  test_readers<iotest::data_error_reader>();
  test_readers<iotest::timeout_reader>();
}

struct zero_reader {
  std::pair<long, std::error_code> read(std::span<uint8_t>)
  { return {0, nil}; }
};

TEST_CASE("Zero reader", "[bufio]") {
  auto z = zero_reader{};
  auto r = reader{z};

  auto c = chan<std::error_code>{};
  auto t = std::thread([&]() {
      auto [_, err] = r.read_byte();
      c << err;
  });

  auto after = time::timer{1s};
  decltype(c)::recv_type err;
  decltype(after)::recv_type v;

  switch (select(
    recv_select_case(c, err),
    recv_select_case(after.c(), v)
  )) {
  case 0:
    CHECK(err == io::error::no_progress);
    break;
  case 1:
    FAIL("test timed out (endless loop in read_byte?)");
    break;
  }

  t.join();
}

class string_reader {
  std::span<std::string_view const> data_;
  long step_ = 0;

 public:
  string_reader(std::span<std::string_view const> data)
      : data_{data} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    if (static_cast<size_t>(step_) < data_.size()) {
      auto s = data_[step_++];
      std::copy(s.begin(), s.end(), p.begin());
      auto n = static_cast<long>(s.size());
      return {n, nil};
    }
    return {0, io::eof};
  }
};

TEST_CASE("Bufio read_rune", "[bufio]") {
  auto test_data = std::vector<std::vector<std::string_view>>{
    {},
    {""},
    {"日", "本語"},
    {"\u65e5", "\u672c", "\u8a9e"},
    {"\U000065e5", "\U0000672c", "\U00008a9e"},
    {"\xe6", "\x97\xa5\xe6", "\x9c\xac\xe8\xaa\x9e"},
    {"Hello", ", ", "World", "!"},
    {"Hello", ", ", "", "World", "!"},
  };
  for (auto const& segments : test_data) {
    auto want = strings::join(segments, "");
    std::string got;
    got.reserve(want.size());
    auto s = string_reader{segments};
    auto rd = reader{s};
    for (;;) {
      auto [r, _, err] = rd.read_rune();
      if (err != nil) {
        if (err != io::eof) {
          return;
        }
        break;
      }
      utf8::encode(r, std::back_inserter(got));
    }
    CHECK(got == want);
  }
}

TEST_CASE("Bufio unread_rune", "[bufio]") {
  auto segments = std::vector<std::string_view>{"Hello, world:", "日本語"};
  auto want = strings::join(segments, "");
  std::string got;
  got.reserve(want.size());
  auto s = string_reader{segments};
  auto r = reader{s};
  for (;;) {
    auto [r1, _1, err1] = r.read_rune();
    if (err1 != nil) {
      CHECK(err1 == io::eof);
      break;
    }
    utf8::encode(r1, std::back_inserter(got));
    REQUIRE(r.unread_rune() == nil);
    auto [r2, _2, err2] = r.read_rune();
    REQUIRE(r1 == r2);
    REQUIRE(err2 == nil);
  }
  CHECK(got == want);
}

TEST_CASE("Bufio no unread_rune after peek", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  br.read_rune();
  br.peek(1);
  CHECK(br.unread_rune() != nil);
}

TEST_CASE("Bufio No unread_byte after peek", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  br.read_byte();
  br.peek(1);
  CHECK(br.unread_byte() != nil);
}

TEST_CASE("Bufio no unread_rune after discard", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  br.read_rune();
  br.discard(1);
  CHECK(br.unread_rune() != nil);
}

TEST_CASE("Bufio no unread_byte after discard", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  br.read_byte();
  br.discard(1);
  CHECK(br.unread_byte() != nil);
}

TEST_CASE("Bufio no unread_rune after write_to", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  auto discard = io::discard{};
  br.write_to(discard);
  CHECK(br.unread_rune() != nil);
}

TEST_CASE("Bufio no unread_byte after write_to", "[bufio]") {
  auto r = strings::reader{"example"};
  auto br = reader{r};
  auto discard = io::discard{};
  br.write_to(discard);
  CHECK(br.unread_byte() != nil);
}

TEST_CASE("Bufio uread_byte", "[bufio]") {
  auto segments = std::vector<std::string_view>{"Hello", "world"};
  auto sr = string_reader{segments};
  auto r = reader{sr};
  std::string got;
  auto want = strings::join(segments, "");
  for (;;) {
    auto [b1, err1] = r.read_byte();
    if (err1 != nil) {
      REQUIRE(err1 == io::eof);
      break;
    }
    got.push_back(b1);
    REQUIRE(r.unread_byte() == nil);
    auto [b2, err2] = r.read_byte();
    REQUIRE(b1 == b2);
    REQUIRE(err2 == nil);
  }
  CHECK(got == want);
}

TEST_CASE("Bufio unread_byte multiple", "[bufio]") {
  auto segments = std::vector<std::string_view>{"Hello", "world"};
  auto data = strings::join(segments, "");
  for (size_t n = 0; n <= data.size(); ++n) {
    auto s = string_reader{segments};
    auto r = reader{s};
    for (size_t i = 0; i < n; ++i) {
      auto [b, err] = r.read_byte();
      REQUIRE(b == data[i]);
      REQUIRE(err == nil);
    }
    if (n > 0) {
      REQUIRE(r.unread_byte() == nil);
    }
    REQUIRE(r.unread_byte() != nil);
  }
}

TEST_CASE("Bufio unread_byte others", "[bufio]") {
  using reader_type = std::function<std::pair<std::vector<uint8_t>, std::error_code>(reader<bytes::buffer>&, uint8_t)>;
  auto readers = std::vector<reader_type>{
    [](reader<bytes::buffer>& r, uint8_t delim) -> std::pair<std::vector<uint8_t>, std::error_code> {
      return r.read_bytes(delim);
    },
    [](reader<bytes::buffer>& r, uint8_t delim) -> std::pair<std::vector<uint8_t>, std::error_code> {
      auto [data, err] = r.read_slice(delim);
      return {{data.begin(), data.end()}, err};
    },
    [](reader<bytes::buffer>& r, uint8_t delim) -> std::pair<std::vector<uint8_t>, std::error_code> {
      auto [data, err] = r.read_string(delim);
      return {{data.begin(), data.end()}, err};
    },
  };

  for (auto read : readers) {
    constexpr size_t n = 10;
    auto buf = bytes::buffer{};
    for (size_t i = 0; i < n; ++i) {
      buf.write_string("abcdefg");
    }

    auto r = reader{buf, min_read_buffer_size};
    auto read_to = [&](uint8_t delim, std::string_view want) {
      auto [data, err] = read(r, delim);
      REQUIRE(bytes::to_string(data) == want);
      REQUIRE(err == nil);
    };

    for (size_t i = 0; i < n; ++i) {
      CAPTURE(i);
      read_to('d', "abcd");
      for (size_t j = 0; j < 3; ++j) {
        REQUIRE(r.unread_byte() == nil);
        read_to('d', "d");
      }
      read_to('g', "efg");
    }

    auto [_, err] = r.read_byte();
    REQUIRE(err == io::eof);
  }
}

TEST_CASE("Bufio unread_rune error", "[bufio]") {
  std::array<uint8_t, 3> buf;
  auto input = std::vector<std::string_view>{"日本語日本語日本語"};
  auto s = string_reader{input};
  auto r = reader{s};
  CHECK(r.unread_rune() != nil);

  auto [_1, _2, err] = r.read_rune();
  CHECK(err == nil);
  CHECK(r.unread_rune() == nil);
  CHECK(r.unread_rune() != nil);

  std::tie(_1, _2, err) = r.read_rune();
  CHECK(err == nil);
  std::tie(_1, err) = r.read(buf);
  CHECK(err == nil);
  CHECK(r.unread_rune() != nil);

  std::tie(_1, _2, err) = r.read_rune();
  CHECK(err == nil);
  for (size_t i = 0; i < buf.size(); ++i) {
    std::tie(_1, err) = r.read_byte();
    CHECK(err == nil);
  }
  CHECK(r.unread_rune() != nil);

  std::tie(_1, _2, err) = r.read_rune();
  CHECK(err == nil);
  std::tie(_1, err) = r.read_byte();
  CHECK(err == nil);
  CHECK(r.unread_byte() == nil);
  CHECK(r.unread_rune() != nil);

  std::tie(_1, _2, err) = r.read_rune();
  CHECK(err == nil);
  auto [_3, err1] = r.read_slice(0);
  CHECK(err1 == io::eof);
  CHECK(r.unread_rune() != nil);
}

TEST_CASE("Bufio unread_rune at EOF", "[bufio]") {
  auto sr = strings::reader{"x"};
  auto r = reader{sr};
  r.read_rune();
  r.read_rune();
  r.unread_rune();
  auto [_1, _2, err] = r.read_rune();
  CHECK(err == io::eof);
}

TEST_CASE("Bufio read/write rune", "[bufio]") {
  constexpr rune nrune = 1000;
  auto byte_buf = bytes::buffer{};
  auto w = writer{byte_buf};
  std::array<uint8_t, utf8::utf_max> buf;
  for (rune r = 0; r < nrune; ++r) {
    auto size = utf8::encode(buf, r);
    auto [nbytes, err] = w.write_rune(r);
    CHECK(nbytes == size);
    CHECK(err == nil);
  }
  w.flush();

  auto r = reader{byte_buf};
  for (rune r1 = 0; r1 < nrune; ++r1) {
    auto size = utf8::encode(buf, r1);
    auto [nr, nbytes, err] = r.read_rune();
    CHECK(nr == r1);
    CHECK(nbytes == size);
    CHECK(err == nil);
  }
}

TEST_CASE("Bufio write invalid rune", "[bufio]") {
  for (auto r : std::vector<rune>{-1, utf8::max_rune + 1}) {
    auto buf = bytes::buffer{};
    auto w = writer{buf};
    w.write_rune(r);
    w.flush();
    CHECK(buf.str() == "\ufffd");
  }
}

TEST_CASE("Bufio writer", "[bufio]") {
  uint8_t data[8192];

  for (size_t i = 0; i < sizeof data; ++i) {
    data[i] = static_cast<uint8_t>(' ' + i%('~'-' '));
  }
  auto w = bytes::buffer{};
  for (size_t i = 0; i < sizeof buffer_sizes.size(); ++i) {
    for (size_t j = 0; j < sizeof buffer_sizes.size(); ++j) {
      auto nwrite = buffer_sizes[i];
      auto bs = buffer_sizes[j];

      // Write nwrite bytes using buffer size bs. Check that the right amount
      // makes it out and the data is correct.

      w.reset();
      auto buf = writer{w, bs};
      CAPTURE(nwrite, bs);
      auto [n, e1] = buf.write(std::span{data, static_cast<size_t>(nwrite)});
      CHECK(e1 == nil);
      CHECK(n == nwrite);
      CHECK(buf.flush() == nil);
      auto written = w.bytes();
      for (auto l = 0; static_cast<size_t>(l) < written.size(); ++l) {
        CHECK(written[l] == data[l]);
      }
    }
  }
}

TEST_CASE("Bufio Writer append", "[bufio]") {
  auto got = bytes::buffer{};
  std::string want;
  auto rn = std::mt19937{std::random_device()()};
  auto w = writer{got, 64};
  for (size_t i = 0; i < 100; ++i) {
    auto b = w.available_buffer();
    CHECK(static_cast<size_t>(w.available()) == b.size());
    auto n = static_cast<int64_t>(std::uniform_int_distribution<int>{0, 1<<30}(rn));
    auto s = strconv::format(n, 10l);
    s.append(" ");
    want.append(s);
    w.write(bytes::to_bytes(s));
  }
  w.flush();
  CHECK(got.str() == want);
}

class error_write_test {
  long n_, m_;
  std::error_code err_;
 public:
  error_write_test(long n, long m, std::error_code err)
      : n_{n} , m_{m} , err_{err} {}
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
    return {static_cast<long>(p.size()) * n_ / m_, err_};
  }
};

TEST_CASE("Bufio write errors", "[bufio]") {
  auto test_cases = std::vector<std::tuple<long, long, std::error_code, std::error_code>>{
    {0, 1, nil, io::error::short_write},
    {1, 2, nil, io::error::short_write},
    {1, 1, nil, nil},
    {0, 1, io::error::closed_pipe, io::error::closed_pipe},
    {1, 2, io::error::closed_pipe, io::error::closed_pipe},
    {1, 1, io::error::closed_pipe, io::error::closed_pipe},
  };
  for (auto [n, m, err, expect] : test_cases) {
    auto w = error_write_test{n, m, err};
    auto buf = writer{w};
    auto [_, err1] = buf.write(bytes::to_bytes("hello world"sv));
    CHECK(err1 == nil);
    for (size_t i = 0; i < 2; ++i) {
      CHECK(buf.flush() == expect);
    }
  }
}

TEST_CASE("Bufio write string", "[bufio]") {
  constexpr long buf_size = 8;
  auto buf = bytes::buffer{};
  auto b = writer{buf, buf_size};
  b.write_string("0");
  b.write_string("123456");
  b.write_string("7890");
  b.write_string("abcdefghijklmnopqrstuvwxy");
  b.write_string("z");
  CHECK(b.flush() == nil);
  CHECK(buf.str() == "01234567890abcdefghijklmnopqrstuvwxyz");
}

class test_string_writer {
  std::string write_;
  std::string write_string_;

 public:
  test_string_writer() = default;
   auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
     write_.append(p.begin(), p.end());
     return {p.size(), nil};
   }
   auto write_string(std::string_view s) -> std::pair<long, std::error_code> {
     write_string_.append(s.begin(), s.end());
     return {s.size(), nil};
   }
   auto check(std::string_view write, std::string_view write_string) {
     CHECK(write == write_);
     CHECK(write_string == write_string_);
   }
};

TEST_CASE("Bufio write string StringWriter", "[bufio]") {
  constexpr long buf_size = 8;
  {
    auto tw = test_string_writer{};
    auto b = writer{tw, buf_size};
    b.write_string("1234");
    tw.check("", "");
    b.write_string("56789012");
    tw.check("12345678", "");
    b.flush();
    tw.check("123456789012", "");
  }
  {
    auto tw = test_string_writer{};
    auto b = writer{tw, buf_size};
    b.write_string("123456789");
    tw.check("", "123456789");
  }
  {
    auto tw = test_string_writer{};
    auto b = writer{tw, buf_size};
    b.write_string("abc");
    tw.check("", "");
    b.write_string("123456789012345");
    tw.check("abc12345", "6789012345");
  }
  {
    auto tw = test_string_writer{};
    auto b = writer{tw, buf_size};
    b.write(bytes::to_bytes(std::string{"abc"}));
    tw.check("", "");
    b.write_string("123456789012345");
    tw.check("abc12345", "6789012345");
  }
}

TEST_CASE("Bufio buffer full", "[bufio]") {
  constexpr auto long_string = "And now, hello, world! It is the time for all good men to come to the aid of their party";
  auto s = strings::reader{long_string};
  auto buf = reader{s, min_read_buffer_size};

  auto [line, err] = buf.read_slice('!');
  CHECK(bytes::to_string(line) == "And now, hello, ");
  CHECK(err == error::buffer_full);

  std::tie(line, err) = buf.read_slice('!');
  CHECK(bytes::to_string(line) == "world!");
  CHECK(err == nil);
}

TEST_CASE("Bufio peek", "[bufio]") {
  std::array<uint8_t, 10> p;
  auto sr = strings::reader{"abcdefghijklmnop"};
  auto buf = reader{sr, min_read_buffer_size};

  auto [s, err1] = buf.peek(1);
  REQUIRE(bytes::to_string(s) == "a");
  REQUIRE(err1 == nil);

  std::tie(s, err1) = buf.peek(4);
  REQUIRE(bytes::to_string(s) == "abcd");
  REQUIRE(err1 == nil);

  std::tie(s, err1) = buf.peek(-1);
  REQUIRE(err1 == error::negative_count);

  std::tie(s, err1) = buf.peek(32);
  REQUIRE(bytes::to_string(s) == "abcdefghijklmnop");
  REQUIRE(err1 == error::buffer_full);

  auto [_, err2] = buf.read(std::span{p.begin(), 3});
  REQUIRE(bytes::to_string(std::span{p.begin(), 3}) == "abc");
  REQUIRE(err2 == nil);

  std::tie(s, err1) = buf.peek(1);
  REQUIRE(bytes::to_string(s) == "d");
  REQUIRE(err1 == nil);

  std::tie(s, err1) = buf.peek(2);
  REQUIRE(bytes::to_string(s) == "de");
  REQUIRE(err1 == nil);

  std::tie(_, err2) = buf.read(std::span{p.begin(), 3});
  REQUIRE(bytes::to_string(p, 3) == "def");
  REQUIRE(err2 == nil);

  std::tie(s, err1) = buf.peek(4);
  REQUIRE(bytes::to_string(s) == "ghij");
  REQUIRE(err1 == nil);

  std::tie(_, err2) = buf.read(p);
  REQUIRE(bytes::to_string(p) == "ghijklmnop");
  REQUIRE(err2 == nil);

  std::tie(s, err1) = buf.peek(0);
  REQUIRE(bytes::to_string(s) == "");
  REQUIRE(err1 == nil);

  std::tie(s, err1) = buf.peek(1);
  REQUIRE(err1 == io::eof);
}

struct data_and_eof_reader {
  std::string r_;
 public:
  data_and_eof_reader(std::string_view r)
      : r_{r} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    long n = std::min(p.size(), r_.size());
    std::copy(r_.begin(), std::next(r_.begin(), n), p.begin());
    return {n, io::eof};
  }
};

TEST_CASE("Bufio peek issue 3022", "[bufio]") {
  std::array<uint8_t, 10> p;
  auto dr = data_and_eof_reader{"abcd"};
  auto buf = reader{dr, 32};

  auto [s, err1] = buf.peek(2);
  REQUIRE(bytes::to_string(s) == "ab");
  REQUIRE(err1 == nil);

  std::tie (s, err1) = buf.peek(4);
  REQUIRE(bytes::to_string(s) == "abcd");
  REQUIRE(err1 == nil);

  auto [n, err2] = buf.read(std::span{p.begin(), 5});
  REQUIRE(bytes::to_string(p, n) == "abcd");
  REQUIRE(err2 == nil);

  std::tie(n, err2) = buf.read(std::span{p.begin(), 1});
  REQUIRE(bytes::to_string(p, n) == "");
  REQUIRE(err2 == io::eof);
}

TEST_CASE("Bufio peek then unread_rune", "[bufio]") {
  auto s = strings::reader{"x"};
  auto r = reader{s};
  r.read_rune();
  r.peek(1);
  r.unread_rune();
  r.read_rune();
}

class test_reader {
  std::string_view data_;
  long stride_;
 public:
  test_reader(std::string_view data, long stride)
      : data_{data}
      , stride_{stride} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    auto n = stride_;
    std::error_code err;
    if (static_cast<size_t>(n) > data_.size()) {
      n = data_.size();
    }
    if (static_cast<size_t>(n) > p.size()) {
      n = p.size();
    }
    std::copy(data_.begin(), std::next(data_.begin(), n), p.begin());
    data_ = data_.substr(n);
    if (data_.size() == 0) {
      err = io::eof;
    }
    return {n, err};
  }
};

auto test_read_line(std::string_view input) {
  std::string_view test_output = "0123456789abcdefghijklmnopqrstuvwxy";
  for (long stride = 1; stride < 2; ++stride) {
    size_t done = 0;
    auto r = test_reader{input, stride};
    auto l = reader{r, static_cast<long>(input.size()+1)};
    for (;;) {
      auto [line, is_prefix, err] = l.read_line();
      if (line.size() > 0) {
        CHECK(err == nil);
      }
      CHECK(is_prefix == false);
      if (err != nil) {
        CHECK(err == io::eof);
        break;
      }
      auto want = test_output.substr(done, line.size());
      CHECK(want == bytes::to_string(line));
      done += line.size();
    }
    CHECK(done == test_output.size());
  }
}

TEST_CASE("Bufio read_line", "[bufio]") {
  std::string_view test_input = "012\n345\n678\n9ab\ncde\nfgh\nijk\nlmn\nopq\nrst\nuvw\nxy";
  std::string_view test_inputrn = "012\r\n345\r\n678\r\n9ab\r\ncde\r\nfgh\r\nijk\r\nlmn\r\nopq\r\nrst\r\nuvw\r\nxy\r\n\n\r\n";
  test_read_line(test_input);
  test_read_line(test_inputrn);
}

TEST_CASE("Bufio line too long", "[bufio]") {
  std::vector<uint8_t> vec;
  for (long i = 0; i < min_read_buffer_size*5/2; ++i) {
    vec.push_back('0'+static_cast<uint8_t>(i%10));
  }
  std::span<uint8_t> data = vec;

  auto buf = bytes::reader{data};
  auto l = reader{buf, min_read_buffer_size};
  auto [line, is_prefix, err] = l.read_line();
  CHECK(bytes::equal(line, bytes::to_bytes(data, min_read_buffer_size)));
  CHECK(is_prefix == true);
  CHECK(err == nil);

  data = data.subspan(line.size());
  std::tie(line, is_prefix, err) = l.read_line();
  CHECK(bytes::equal(line, data.subspan(0, min_read_buffer_size)));
  CHECK(is_prefix == true);
  CHECK(err == nil);

  data = data.subspan(line.size());
  std::tie(line, is_prefix, err) = l.read_line();
  CHECK(bytes::equal(line, data.subspan(0, min_read_buffer_size/2)));
  CHECK(is_prefix == false);
  CHECK(err == nil);

  std::tie(line, is_prefix, err) = l.read_line();
  CHECK(is_prefix == false);
  CHECK(err != nil);
}

TEST_CASE("Bufio read after lines", "[bufio]") {
  std::string_view line1 = "this is line1";
  std::string_view rest_data = "this is line2\nthis is line3\n";
  auto inbuf = bytes::buffer{};
  inbuf.write_string(line1);
  inbuf.write_string("\n");
  inbuf.write_string(rest_data);
  auto outbuf = bytes::buffer{};
  long max_line_length = line1.size() + rest_data.size()/2;
  auto l = reader{inbuf, max_line_length};

  auto [line, is_prefix, err1] = l.read_line();
  CHECK(bytes::to_string(line) == line1);
  CHECK(is_prefix == false);
  CHECK(err1 == nil);

  auto [n, err2] = io::copy(outbuf, l);
  CHECK(outbuf.str() == rest_data);
  CHECK(static_cast<size_t>(n) == rest_data.size());
  CHECK(err2 == nil);
}

TEST_CASE("Bufio read empty buffer", "[bufio]") {
  auto buf = bytes::buffer{};
  auto l = reader{buf, min_read_buffer_size};
  auto [line, is_prefix, err] = l.read_line();
  CHECK(err == io::eof);
}

TEST_CASE("Bufio lines after read", "[bufio]") {
  auto buf = bytes::buffer{"foo"};
  auto l = reader{buf, min_read_buffer_size};
  auto [_, err1] = io::read_all(l);
  CHECK(err1 == nil);
  auto [line, is_prefix, err2] = l.read_line();
  CHECK(err2 == io::eof);
}

TEST_CASE("Bufio read_line non-nil line or error", "[bufio]") {
  auto str = strings::reader{"line 1\n"};
  auto r = reader{str};
  for (long i = 0; i < 2; ++i) {
    auto [l, _, err] = r.read_line();
    if (l != nil) {
      REQUIRE(err == nil);
    }
  }
}

TEST_CASE("Bufio read_line newlines", "[bufio]") {
  auto test_cases = std::vector<std::tuple<
    std::string_view,
    std::vector<std::tuple<std::string_view, bool, std::error_code>>
  >>{
    {"012345678901234\r\n012345678901234\r\n", {
      {"012345678901234", true, nil},
      {nil, false, nil},
      {"012345678901234", true, nil},
      {nil, false, nil},
      {nil, false, io::eof},
    }},
    {"0123456789012345\r012345678901234\r", {
      {"0123456789012345", true, nil},
      {"\r012345678901234", true, nil},
      {"\r", false, nil},
      {nil, false, io::eof},
    }},
  };
  for (auto [input, values] : test_cases) {
    auto s = strings::reader{input};
    auto r = reader{s, min_read_buffer_size};
    for (auto [exp_line, exp_is_prefix, exp_err] : values) {
      auto [line, is_prefix, err] = r.read_line();
      CHECK(bytes::to_string(line) == exp_line);
      CHECK(is_prefix == exp_is_prefix);
      CHECK(err == exp_err);
    }
  }
}

template <typename T> requires io::ReaderFunc<T>
class only_reader {
  T& rd_;

 public:
  only_reader(T& rd)
      : rd_{rd} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    return read(rd_, p);
  }
};

template <typename T> requires io::WriterFunc<T>
class only_writer {
  T& wr_;

 public:
  only_writer(T& wr)
      : wr_{wr} {}
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
    using io::write;
    return write(wr_, p);
  }
};

auto create_test_input(long n) -> std::vector<uint8_t> {
  auto input = std::vector<uint8_t>{};
  input.resize(n);
  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = static_cast<uint8_t>(i % 251);
    if (i%101 == 0) {
      input[i] ^= static_cast<uint8_t>(i / 101);
    }
  }
  return input;
}

TEST_CASE("Bufio Reader write_to", "[bufio]") {
  auto input = create_test_input(8192);
  auto b = bytes::reader{input};
  auto o = only_reader{b};
  auto r = reader{o};
  auto w = bytes::buffer{};
  auto [n, err] = r.write_to(w);
  REQUIRE(n == static_cast<long>(input.size()));
  REQUIRE(err == nil);
  for (long i = 0; i < w.size(); ++i) {
    CHECK(w[i] == input[i]);
  }
}

class error_reader_writer {
  long rn_, wn_;
  std::error_code rerr_, werr_;
 public:
  error_reader_writer(long rn, long wn, std::error_code rerr, std::error_code werr)
      : rn_{rn} , wn_{wn} , rerr_{rerr} , werr_{werr} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code>
  { return {p.size() * rn_, rerr_}; }
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code>
  { return {p.size() * wn_, werr_}; }
};

TEST_CASE("Bufio Reader write_to errors", "[bufio]") {
  auto test_cases = std::vector<std::tuple<error_reader_writer, std::error_code>>{
    {{1, 0, nil, io::error::closed_pipe}, io::error::closed_pipe},
    {{0, 1, io::error::closed_pipe, nil}, io::error::closed_pipe},
    {{0, 0, io::error::unexpected_eof, io::error::closed_pipe}, io::error::closed_pipe},
    {{0, 0, io::eof, nil}, nil},
  };
  for (auto& [rw, exp] : test_cases) {
    auto r = reader{rw};
    auto [_, err] = r.write_to(rw);
    CHECK(err == exp);
  }
}

template <typename T, typename U> requires io::Writer<T> && io::Reader<U>
auto test_writer_read_from() -> void {
  auto b = bytes::buffer{};
  auto t = T{b};
  auto w = writer{t};

  auto input = create_test_input(8192);
  auto br = bytes::reader{input};
  auto r = U{br};

  auto [n, err] = w.read_from(r);
  CHECK(n == static_cast<long>(input.size()));
  CHECK(err == nil);
  CHECK(w.flush() == nil);
  CHECK(b.str() == bytes::to_string(input));
}

TEST_CASE("Bufio Writer read_from", "[bufio]") {
  test_writer_read_from<
    only_writer<bytes::buffer>,
    testing::iotest::data_error_reader<bytes::reader>
  >();
  test_writer_read_from<
    passthru_writer<bytes::buffer>,
    testing::iotest::data_error_reader<bytes::reader>
  >();
  test_writer_read_from<
    only_writer<bytes::buffer>,
    passthru_reader<bytes::reader>
  >();
  test_writer_read_from<
    passthru_writer<bytes::buffer>,
    passthru_reader<bytes::reader>
  >();
}

TEST_CASE("Bufio Writer read_from errors", "[bufio]") {
  auto test_cases = std::vector<std::tuple<error_reader_writer, std::error_code>>{
    {{0, 1, io::eof, nil}, nil},
    {{1, 1, io::eof, nil}, nil},
    {{0, 1, io::error::closed_pipe, nil}, io::error::closed_pipe},
    {{0, 0, io::error::closed_pipe, io::error::short_write}, io::error::closed_pipe},
    {{1, 0, nil, io::error::short_write}, io::error::short_write},
  };
  for (auto& [rw, exp] : test_cases) {
    auto w = writer{rw};
    auto [_, err] = w.read_from(rw);
    CHECK(err == exp);
  }
}

class write_counting_discard {
  long count_ = 0;

 public:
  write_counting_discard() = default;
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
    ++count_;
    return {static_cast<long>(p.size()), nil};
  }

  auto count() const { return count_; }
};

TEST_CASE("Bufio Writer read_from counts", "[bufio]") {
  auto w0 = write_counting_discard{};
  auto b0 = writer{w0, 1234};

  b0.write_string(strings::repeat("x", 1000));
  REQUIRE(w0.count() == 0);

  b0.write_string(strings::repeat("x", 200));
  REQUIRE(w0.count() == 0);

  b0.write_string(strings::repeat("x", 30));
  REQUIRE(w0.count() == 0);

  b0.write_string(strings::repeat("x", 9));
  REQUIRE(w0.count() == 1);

  auto w1 = write_counting_discard{};
  auto b1 = writer{w1, 1234};

  b1.write_string(strings::repeat("x", 1200));
  b1.flush();
  REQUIRE(w1.count() == 1);

  b1.write_string(strings::repeat("x", 89));
  REQUIRE(w1.count() == 1);

  auto s1 = strings::repeat("x", 700);
  auto r1 = strings::reader{s1};
  auto o1 = only_reader{r1};
  io::copy(b1, o1);
  REQUIRE(w1.count() == 1);

  auto s2 = strings::repeat("x", 600);
  auto r2 = strings::reader{s2};
  auto o2 = only_reader{r2};
  io::copy(b1, o2);
  REQUIRE(w1.count() == 2);

  b1.flush();
  REQUIRE(w1.count() == 3);
}

struct negative_reader {
  auto read(std::span<uint8_t>) -> std::pair<long, std::error_code> { return {-1, nil}; }
};

TEST_CASE("Bufio negative read", "[bufio]") {
  std::array<uint8_t, 100> buf;
  auto r = negative_reader{};
  auto b = reader{r};
  CHECK_THROWS_AS([&]() {
    b.read(buf);
  }(), std::system_error);
}

struct error_then_good_reader {
  bool did_err = false;
  long nread = 0;
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    ++nread;
    if (!did_err) {
      did_err = true;
      return {0, testing::error::test_error};
    }
    return {static_cast<long>(p.size()), nil};
  }
};

TEST_CASE("Bufio Reader clear error", "[bufio]") {
  auto r = error_then_good_reader{};
  auto b = reader{r};
  std::vector<uint8_t> buf;
  buf.resize(1);
  auto [_, err] = b.read(nil);
  REQUIRE(err == nil);
  std::tie(_, err) = b.read(buf);
  REQUIRE(err == testing::error::test_error);
  std::tie(_, err) = b.read(nil);
  REQUIRE(err == nil);
  std::tie(_, err) = b.read(buf);
  REQUIRE(err == nil);
  CHECK(r.nread == 2);
}

TEST_CASE("Bufio Writer read_from while full", "[bufio]") {
  auto buf = bytes::buffer{};
  auto w = writer{buf, 10};

  auto [n, err] = w.write(bytes::to_bytes("0123456789"sv));
  REQUIRE(n == 10);
  REQUIRE(err == nil);

  auto s = strings::reader{"abcdef"};
  auto [n2, err2] = w.read_from(s);
  REQUIRE(n2 == 6);
  REQUIRE(err2 == nil);
}

template <typename T> requires io::ReaderFunc<T>
class empty_then_non_empty_reader {
  T* r_;
  long n_;
 public:
  empty_then_non_empty_reader(T& r, long n)
      : r_{std::addressof(r)}
      , n_{n} {}
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    if (n_ <= 0) {
      return read(*r_, p);
    }
    --n_;
    return {0, nil};
  }
};

TEST_CASE("Bufio Writer read_from until EOF", "[bufio]") {
  auto buf = bytes::buffer{};
  auto w = writer{buf, 5};

  auto [n, err] = w.write(bytes::to_bytes("0123"sv));
  REQUIRE(n == 4);
  REQUIRE(err == nil);

  auto s = strings::reader{"abcd"};
  auto r = empty_then_non_empty_reader{s, 3};
  auto [n2, err2] = w.read_from(r);
  REQUIRE(n2 == 4);
  REQUIRE(err == nil);

  w.flush();
  REQUIRE(buf.str() == "0123abcd");
}

TEST_CASE("Bufio Writer read_from error::no_progress", "[bufio]") {
  auto buf = bytes::buffer{};
  auto w = writer{buf, 5};

  auto [n, err] = w.write(bytes::to_bytes("0123"sv));
  REQUIRE(n == 4);
  REQUIRE(err == nil);

  auto s = strings::reader{"abcd"};
  auto r = empty_then_non_empty_reader{s, 100};
  auto [n2, err2] = w.read_from(r);
  REQUIRE(n2 == 0);
  REQUIRE(err2 == io::error::no_progress);
}

class read_from_writer {
  std::vector<uint8_t> buf_;
  long write_bytes_ = 0;
  long read_from_bytes_ = 0;
 public:
  read_from_writer() = default;
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
    buf_.insert(buf_.end(), p.begin(), p.end());
    write_bytes_ += p.size();
    return {static_cast<long>(p.size()), nil};
  }
  template <typename T> requires io::Reader<T>
  auto read_from(T& r) -> std::pair<int64_t, std::error_code> {
    auto [b, err] = io::read_all(r);
    buf_.insert(buf_.end(), b.begin(), b.end());
    read_from_bytes_ += b.size();
    return {static_cast<int64_t>(b.size()), err};
  }
  auto buf() const -> std::span<uint8_t const> { return std::span{buf_}; }
  auto write_bytes() const -> long { return write_bytes_; }
  auto read_from_bytes() const -> long { return read_from_bytes_; }
};

TEST_CASE("Bufio Writer read_from with buffered data", "[bufio]") {
  constexpr long buf_size = 16;

  auto data = create_test_input(64);
  auto input = std::span{data};
  auto rfw = read_from_writer{};
  auto w = writer{rfw, buf_size};

  constexpr long write_size = 8;
  auto [n, err] = w.write(input.subspan(0, write_size));
  CHECK(n == write_size);
  CHECK(err == nil);

  auto b = bytes::reader{input.subspan(write_size)};
  std::tie(n, err) = w.read_from(b);
  CHECK(n == static_cast<long>(input.subspan(write_size).size()));
  CHECK(err == nil);
  CHECK(w.flush() == nil);
  CHECK(rfw.write_bytes() == buf_size);
  CHECK(rfw.read_from_bytes() == static_cast<long>(input.size()-buf_size));
}

TEST_CASE("Bufio read zero", "[bufio]") {
  for (auto size : std::vector<long>{100, 2}) {
    auto r1 = strings::reader{"abc"};
    auto s = strings::reader{"def"};
    auto r2 = empty_then_non_empty_reader{s, 1};
    auto r = io::multi_reader{r1, r2};
    auto br = reader{r, size};
    auto want = [&](std::string_view s, std::error_code want_err) {
      std::array<uint8_t, 50> p;
      auto [n, err] = br.read(p);
      CHECK(n == static_cast<long>(s.size()));
      CHECK(err == want_err);
      CHECK(bytes::to_string(p, n) == s);
    };
    want("abc", nil);
    want("", nil);
    want("def", nil);
    want("", io::eof);
  }
}

TEST_CASE("Bufio Reader reset", "[bufio]") {
  auto s1 = strings::reader{"foo foo"};
  auto s2 = strings::reader{"bar bar"};
  auto s3 = strings::reader{"bar bar"};

  auto r = reader{s1};
  std::array<uint8_t, 3> buf;
  r.read(buf);
  CHECK(bytes::to_string(buf) == "foo");

  r.reset(s2);
  auto [all, err] = io::read_all(r);
  CHECK(bytes::to_string(all) == "bar bar");
  CHECK(err == nil);

  r = reader{s3};
  std::tie(all, err) = io::read_all(r);
  CHECK(bytes::to_string(all) == "bar bar");
  CHECK(err == nil);
}

TEST_CASE("Bufio Writer reset", "[bufio]") {
  auto buf1 = bytes::buffer{}, buf2 = bytes::buffer{}, buf3 = bytes::buffer{};
  auto w = writer{buf1};
  w.write_string("foo");

  w.reset(buf2);
  w.write_string("bar");
  w.flush();
  CHECK(buf1.str() == "");
  CHECK(buf2.str() == "bar");

  w = writer{buf3};
  w.write_string("bar");
  w.flush();
  CHECK(buf1.str() == "");
  CHECK(buf3.str() == "bar");
}

template <typename T>
class scripted_reader {
  std::vector<T> steps_;
  size_t i_ = 0;

 public:
  template <typename... Ts>
  scripted_reader(Ts&&... steps)
      : steps_{std::move(steps)...} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    if (i_ == steps_.size()) {
      throw std::runtime_error{"Too many Read calls on scripted Reader. No steps remain."};
    }
    return steps_[i_++](p);
  }
};

template <typename T> requires io::Reader<T>
auto test_reader_discard(T& r, std::string_view name, long buf_size, size_t peek_size, long n, long want, std::error_code want_err, long want_buffered) {
  auto br = reader{r, buf_size};
  CAPTURE(name);
  if (peek_size > 0) {
    auto [peek_buf, err] = br.peek(peek_size);
    CHECK(peek_buf.size() == peek_size);
    CHECK(err == nil);
  }
  auto [discarded, err] = br.discard(n);
  CHECK(fmt::sprint(err) == fmt::sprint(want_err));
  CHECK(discarded == want);
  CHECK(br.buffered() == want_buffered);
}

TEST_CASE("Bufio Reader discard", "[bufio]") {
  SECTION("strings::reader") {
    auto test_cases = std::vector<std::tuple<
      std::string_view,
      strings::reader,
      long,
      size_t,
      long,
      long,
      std::error_code,
      long
    >>{
      {
        "normal case",
        strings::reader{"abcdefghijklmnopqrstuvwxyz"},
        0,    // buf_size
        16,   // peek_size
        6,    // n
        6,    // want
        nil,  // want_err
        10    // want_buffered
      },
      {
        "discard causing read",
        strings::reader{"abcdefghijklmnopqrstuvwxyz"},
        0,    // buf_size
        0,    // peek_size
        6,    // n
        6,    // want
        nil,  // want_err
        10    // want_buffered
      },
      {
        "discard all without peek",
        strings::reader{"abcdefghijklmnopqrstuvwxyz"},
        0,    // buf_size
        0,    // peek_size
        26,   // n
        26,   // want
        nil,  // want_err
        0     // want_buffered
      },
      {
        "discard more than end",
        strings::reader{"abcdefghijklmnopqrstuvwxyz"},
        0,        // buf_size
        0,        // peek_size
        27,       // n
        26,       // want
        io::eof,  // want_err
        0         // want_buffered
      },
    };
    for (auto& [name, r, buf_size, peek_size, n, want, want_err, want_buffered] : test_cases) {
      test_reader_discard(r, name, buf_size, peek_size, n, want, want_err, want_buffered);
    }
  }
  SECTION("scripted_reader") {
    auto test_cases = std::vector<std::tuple<
      std::string_view,
      scripted_reader<std::pair<long, std::error_code>(*)(std::span<uint8_t>)>,
      long,
      size_t,
      long,
      long,
      std::error_code,
      long
    >>{
      {
        "fill error, discard less",
        {[](std::span<uint8_t> p) -> std::pair<long, std::error_code> {
          if (p.size() < 5) {
            throw std::runtime_error{"unexpected small read"};
          }
          return {5, testing::error::test_error};
        }},
        0,    // buf_size
        0,    // peek_size
        4,    // n
        4,    // want
        nil,  // want_err
        1     // want_buffered
      },
      {
        "fill error, discard equal",
        {[](std::span<uint8_t> p) -> std::pair<long, std::error_code> {
          if (p.size() < 5) {
            throw std::runtime_error{"unexpected small read"};
          }
          return {5, testing::error::test_error};
        }},
        0,    // buf_size
        0,    // peek_size
        5,    // n
        5,    // want
        nil,  // want_err
        0     // want_buffered
      },
      {
        "fill error, discard more",
        {[](std::span<uint8_t> p) -> std::pair<long, std::error_code> {
          if (p.size() < 5) {
            throw std::runtime_error{"unexpected small read"};
          }
          return {5, testing::error::test_error};
        }},
        0,                           // buf_size
        0,                           // peek_size
        6,                           // n
        5,                           // want
        testing::error::test_error,  // want_err
        0                            // want_buffered
      },
      {
        "discard zero",
        {},
        0,    // buf_size
        0,    // peek_size
        0,    // n
        0,    // want
        nil,  // want_err
        0     // want_buffered
      },
      {
        "discard negative",
        {},
        0,                      // buf_size
        0,                      // peek_size
        -1,                     // n
        0,                      // want
        error::negative_count,  // want_err
        0                       // want_buffered
      },
    };
    for (auto& [name, r, buf_size, peek_size, n, want, want_err, want_buffered] : test_cases) {
      test_reader_discard(r, name, buf_size, peek_size, n, want, want_err, want_buffered);
    }
  }
}

TEST_CASE("Bufio Reader size", "[bufio]") {
  auto b = bytes::buffer{};
  CHECK(reader{b}.size() == 4096);
  CHECK(reader{b, 1234}.size() == 1234);
}

TEST_CASE("Bufio Writer size", "[bufio]") {
  auto b = bytes::buffer{};
  CHECK(writer{b}.size() == 4096);
  CHECK(writer{b, 1234}.size() == 1234);
}

class eof_reader {
  std::vector<uint8_t> data_;
  std::span<uint8_t> buf_;

 public:
  eof_reader(std::vector<uint8_t> data)
      : data_{std::move(data)}
      , buf_{data_} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    auto n = std::min(buf_.size(), p.size());
    std::copy(buf_.begin(), std::next(buf_.begin(), n), p.begin());
    buf_ = buf_.subspan(n);
    if (n == 0 || n == buf_.size()) {
      return {n, io::eof};
    }
    return {n, nil};
  }

  size_t size() const { return buf_.size(); }
};

TEST_CASE("Bufio partial read EOF", "[bufio]") {
  auto eof_r = eof_reader{std::vector<uint8_t>(10, 0)};
  auto r = reader{eof_r};

  auto dst = std::vector<uint8_t>(5, 0);
  auto [read, err] = r.read(dst);
  REQUIRE(static_cast<size_t>(read) == dst.size());
  REQUIRE(err == nil);
  REQUIRE(eof_r.size() == 0);
  REQUIRE(r.buffered() == 5);

  std::tie(read, err) = r.read({});
  REQUIRE(read == 0);
  REQUIRE(err == nil);
}

struct writer_with_read_from_error {
  template <typename T> requires io::Reader<T>
  auto read_from(T&) -> std::pair<int64_t, std::error_code>
  { return {0, testing::error::test_error}; }

  auto write(std::span<uint8_t const>) -> std::pair<long, std::error_code>
  { return {10, nil}; }
};

TEST_CASE("Bufio Writer read_from must set underlying error", "[bufio]") {
  auto w = writer_with_read_from_error{};
  auto wr = writer{w};

  auto s = strings::reader{"test2"};
  auto [_1, err1] = wr.read_from(s);
  REQUIRE(err1 != nil);

  auto b = std::array<uint8_t, 3>{'a', 'b', 'c'};
  auto [_2, err2] = wr.write(b);
  REQUIRE(err2 != nil);
}

struct error_only_writer {
  auto write(std::span<uint8_t const>) -> std::pair<long, std::error_code>
  { return {0, testing::error::test_error}; }
};

TEST_CASE("Bufio Writer read_from must return underlying error", "[bufio]") {
  auto w = error_only_writer{};
  auto wr = writer{w};
  std::string_view s1 = "test1";

  auto [_, err] = wr.write_string(s1);
  REQUIRE(err == nil);

  err = wr.flush();
  REQUIRE(err != nil);

  int64_t n;
  auto s2 = strings::reader{"test2"};
  std::tie(n, err) = wr.read_from(s2);
  REQUIRE(err != nil);
  REQUIRE(static_cast<size_t>(wr.buffered()) == s1.size());
}

#if defined(CATCH_CONFIG_ENABLE_BENCHMARKING)

TEST_CASE("Bufio benchmarks", "[!benchmark]") {
  BENCHMARK_ADVANCED("Reader copy optimal")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto src = reader{src_buf};
    auto dst_buf = bytes::buffer{};
    auto dst = only_writer{dst_buf};
    meter.measure([&]() {
      src_buf.reset();
      src.reset(src_buf);
      dst_buf.reset();
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Reader copy sub-optimal")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto r = only_reader{src_buf};
    auto src = reader{r};
    auto dst_buf = bytes::buffer{};
    auto dst = only_writer{dst_buf};
    meter.measure([&]() {
      src_buf.reset();
      src.reset(r);
      dst_buf.reset();
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Reader copy no write_to")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto src_reader = reader{src_buf};
    auto src = only_reader{src_reader};
    auto dst_buf = bytes::buffer{};
    auto dst = only_writer{dst_buf};
    meter.measure([&]() {
      src_buf.reset();
      src_reader.reset(src_buf);
      dst_buf.reset();
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Reader write_to optimal")(Catch::Benchmark::Chronometer meter) {
    constexpr long buf_size = 16 << 10;
    std::array<uint8_t, buf_size> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::reader{buf};
    auto r = only_reader{src_buf};
    auto src_reader = reader{r};
    auto discard = io::discard{};
    meter.measure([&]() {
      src_buf.seek(0, io::seek_start);
      src_reader.reset(r);
      auto [n, err] = src_reader.write_to(discard);
      REQUIRE(n == buf_size);
      REQUIRE(err == nil);
    });
  };

  BENCHMARK_ADVANCED("Reader read_string")(Catch::Benchmark::Chronometer meter) {
    auto r = strings::reader{"       foo       foo        42        42        42        42        42        42        42        42       4.2       4.2       4.2       4.2\n"};
    auto buf = reader{r};
    meter.measure([&]() {
      r.seek(0, io::seek_start);
      buf.reset(r);
      auto [_, err] = buf.read_string('\n');
      REQUIRE(err == nil);
    });
  };

  BENCHMARK_ADVANCED("Writer copy optimal")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto src = only_reader{src_buf};
    auto dst_buf = bytes::buffer{};
    auto dst = writer{dst_buf};
    meter.measure([&]() {
      src_buf.reset();
      dst_buf.reset();
      dst.reset(dst_buf);
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Writer copy sub-optimal")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto src = only_reader{src_buf};
    auto dst_buf = bytes::buffer{};
    auto w = only_writer{dst_buf};
    auto dst = writer{w};
    meter.measure([&]() {
      src_buf.reset();
      dst_buf.reset();
      dst.reset(w);
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Writer copy no read_from")(Catch::Benchmark::Chronometer meter) {
    std::array<uint8_t, 8192> buf;
    std::fill(buf.begin(), buf.end(), 1);
    auto src_buf = bytes::buffer{buf};
    auto src = only_reader{src_buf};
    auto dst_buf = bytes::buffer{};
    auto w = writer{dst_buf};
    auto dst = only_writer{w};
    meter.measure([&]() {
      src_buf.reset();
      dst_buf.reset();
      w.reset(dst_buf);
      io::copy(dst, src);
    });
  };

  BENCHMARK_ADVANCED("Reader empty")(Catch::Benchmark::Chronometer meter) {
    auto str = strings::repeat("x", 16<<10);
    auto discard = io::discard{};
    meter.measure([&]() {
      auto r = strings::reader{str};
      auto br = reader{r};
      auto [n, err] = io::copy(discard, br);
      REQUIRE(static_cast<size_t>(n) == str.size());
      REQUIRE(err == nil);
    });
  };

  BENCHMARK_ADVANCED("Writer empty")(Catch::Benchmark::Chronometer meter) {
    auto str = strings::repeat("x", 1<<10);
    auto bs = bytes::to_bytes(str);
    auto discard = io::discard{};
    meter.measure([&]() {
      auto bw = writer{discard};
      bw.flush();
      bw.write_byte('a');
      bw.flush();
      bw.write_rune('B');
      bw.flush();
      bw.write(bs);
      bw.flush();
      bw.write_string(str);
      bw.flush();
    });
  };

  BENCHMARK_ADVANCED("Writer flush")(Catch::Benchmark::Chronometer meter) {
    auto discard = io::discard{};
    auto bw = writer{discard};
    auto str = strings::repeat("x", 50);
    meter.measure([&]() {
      bw.write_string(str);
      bw.flush();
    });
  };
}

#endif  // defined(CATCH_CONFIG_ENABLE_BENCHMARKING)

}  // namespace bongo::bufio
