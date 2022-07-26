// Copyright The Go Authors.

#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/bytes.h"
#include "bongo/io.h"
#include "bongo/strings.h"

namespace bongo::io {

using namespace std::string_view_literals;

struct buffer : public bytes::buffer {
  void read_from();
  void write_to();
};

TEST_CASE("Copy", "[io]") {
  auto rb = buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  copy(wb, rb);
  CHECK(wb.str() == "hello, world.");
}

TEST_CASE("Copy negative", "[io]") {
  auto rb = buffer{};
  auto wb = buffer{};
  rb.write_string("hello");
  auto lr = limited_reader{rb, -1};
  copy(wb, lr);
  CHECK(wb.str() == "");

  copy_n(wb, rb, -1);
  CHECK(wb.str() == "");
}

TEST_CASE("Copy buffer", "[io]") {
  auto rb = buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  auto buf = std::vector<uint8_t>(1);
  copy_buffer(wb, rb, buf);
  CHECK(wb.str() == "hello, world.");
}

TEST_CASE("Copy buffer nil", "[io]") {
  auto rb = buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  copy_buffer(wb, rb, nil);
  CHECK(wb.str() == "hello, world.");
}

TEST_CASE("Copy read from", "[io]") {
  auto rb = buffer{};
  auto wb = bytes::buffer{};
  rb.write_string("hello, world.");
  copy(wb, rb);
  CHECK(wb.str() == "hello, world.");
}

TEST_CASE("Copy write to", "[io]") {
  auto rb = bytes::buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  copy(wb, rb);
  CHECK(wb.str() == "hello, world.");
}

struct write_to_checker : public bytes::buffer {
  bool write_to_called = false;
  template <Writer T>
  std::pair<int64_t, std::error_code> write_to(T& t) {
    write_to_called = true;
    return bytes::buffer::write_to(t);
  }
};

TEST_CASE("Copy priority", "[io]") {
  auto rb = write_to_checker{};
  auto wb = bytes::buffer{};
  rb.write_string("hello, world.");
  copy(wb, rb);
  CHECK(wb.str() == "hello, world.");
  CHECK(rb.write_to_called == true);
}

struct zero_err_reader {
  std::error_code err;
  std::pair<int, std::error_code> read(std::span<uint8_t> p) {
    return {p.size(), err};
  }
};

struct err_writer {
  std::error_code err;
  std::pair<int, std::error_code> write(std::span<uint8_t const>) {
    return {0, err};
  }
};

TEST_CASE("Copy read error/write error", "[io]") {
  auto r = zero_err_reader{std::make_error_code(std::errc::not_a_socket)};
  auto w = err_writer{std::make_error_code(std::errc::not_a_stream)};
  auto [n, err] = copy(w, r);
  CHECK(n == 0);
  CHECK(err == w.err);
}

TEST_CASE("Copy N", "[io]") {
  auto rb = buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  copy_n(wb, rb, 5);
  CHECK(wb.str() == "hello");
}

TEST_CASE("Copy N read_from", "[io]") {
  auto rb = buffer{};
  auto wb = bytes::buffer{};
  rb.write_string("hello, world.");
  copy_n(wb, rb, 5);
}

TEST_CASE("Copy N write_to", "[io]") {
  auto rb = bytes::buffer{};
  auto wb = buffer{};
  rb.write_string("hello, world.");
  copy_n(wb, rb, 5);
}

template <Writer T>
struct no_read_from {
  T& t;

  no_read_from(T& t_)
      : t{t_} {}

  std::pair<int, std::error_code> write(std::span<uint8_t const> p) {
    return t.write(p);
  }
};

struct wanted_and_err_reader {
  std::pair<int, std::error_code> read(std::span<uint8_t> p) {
    return {p.size(), std::make_error_code(std::errc::not_a_socket)};
  }
};

TEST_CASE("Copy N EOF", "[io]") {
  auto b = bytes::buffer{};
  auto r = strings::reader{};
  auto w = no_read_from{b};
  auto e = wanted_and_err_reader{};

  r.reset("foo");
  auto [n, err] = copy_n(w, r, 3);
  CHECK(n == 3);
  CHECK(err == nil);

  r.reset("foo");
  std::tie(n, err) = copy_n(w, r, 4);
  CHECK(n == 3);
  CHECK(err == eof);

  r.reset("foo");
  std::tie(n, err) = copy_n(b, r, 3);
  CHECK(n == 3);
  CHECK(err == nil);

  r.reset("foo");
  std::tie(n, err) = copy_n(b, r, 4);
  CHECK(n == 3);
  CHECK(err == io::eof);

  std::tie(n, err) = copy_n(b, e, 5);
  CHECK(n == 5);
  CHECK(err == nil);

  std::tie(n, err) = copy_n(w, e, 5);
  CHECK(n == 5);
  CHECK(err == nil);
}

template <Reader T>
void test_read_at_least(T& rb, std::error_code want) {
  rb.write(bytes::to_bytes("0123"sv));
  auto buf = std::vector<uint8_t>(2);

  auto [n, err] = read_at_least(rb, buf, 2);
  CHECK(n == 2);
  CHECK(err == nil);

  std::tie(n, err) = read_at_least(rb, buf, 4);
  CHECK(n == 0);
  CHECK(err == error::short_buffer);

  std::tie(n, err) = read_at_least(rb, buf, 1);
  CHECK(n == 2);
  CHECK(err == nil);

  std::tie(n, err) = read_at_least(rb, buf, 2);
  CHECK(n == 0);
  CHECK(err == eof);

  rb.write(bytes::to_bytes("4"sv));
  std::tie(n, err) = read_at_least(rb, buf, 2);
  CHECK(n == 1);
  CHECK(err == want);
}

TEST_CASE("Read at least", "[io]") {
  auto rb = bytes::buffer{};
  test_read_at_least(rb, error::unexpected_eof);
}

struct data_and_error_buffer : bytes::buffer {
  std::error_code err;
  std::pair<int, std::error_code> read(std::span<uint8_t> p) {
    auto [n, err2] = bytes::buffer::read(p);
    if (n > 0 && size() == 0 && err2 == nil) {
      return {n, err};
    }
    return {n, err2};
  }
};

TEST_CASE("Read at least with data and EOF", "[io]") {
  auto rb = data_and_error_buffer{};
  rb.err = eof;
  test_read_at_least(rb, error::unexpected_eof);
}

TEST_CASE("Read at least with data and error", "[io]") {
  auto rb = data_and_error_buffer{};
  rb.err = std::make_error_code(std::errc::not_a_socket);
  test_read_at_least(rb, std::make_error_code(std::errc::not_a_socket));
}

TEST_CASE("Tee reader", "[io]") {
  auto src = std::string_view{"hello, world"};
  auto dst = std::vector<uint8_t>(src.size());
  auto rb = bytes::buffer{src};
  auto wb = bytes::buffer{};
  auto r1 = tee_reader{rb, wb};

  auto [n, err] = read_full(r1, dst);
  CHECK(static_cast<size_t>(n) == src.size());
  CHECK(err == nil);

  std::tie(n, err) = r1.read(dst);
  CHECK(n == 0);
  CHECK(err == eof);

  rb = bytes::buffer{src};
  auto [pr, pw] = make_pipe();
  pr.close();
  auto r2 = tee_reader{rb, pw};
  std::tie(n, err) = read_full(r2, dst);
  CHECK(n == 0);
  CHECK(err == error::closed_pipe);
}

TEST_CASE("Section reader read_at", "[io]") {
  struct test_case {
    std::string_view data;
    int off;
    int n;
    int buf_len;
    int at;
    std::string_view exp;
    std::error_code err;
  };
  auto const dat = std::string_view{"a long sample data, 1234567890"};
  int const size = static_cast<int>(dat.size());
  auto test_cases = std::vector<test_case>{
    {"",  0, 10, 2, 0, "", eof},
    {dat, 0, size, 0, 0, "", nil},
    {dat, size, 1, 1, 0, "", eof},
    {dat, 0, size + 2, size, 0, dat, nil},
    {dat, 0, size, size / 2, 0, dat.substr(0, size/2), nil},
    {dat, 0, size, size, 0, dat, nil},
    {dat, 0, size, size / 2, 2, dat.substr(2, size/2), nil},
    {dat, 3, size, size / 2, 2, dat.substr(5, size/2), nil},
    {dat, 3, size / 2, size/2 - 2, 2, dat.substr(5, size/2-2), nil},
    {dat, 3, size / 2, size/2 + 2, 2, dat.substr(5, size/2-2), eof},
    {dat, 0, 0, 0, -1, "", eof},
    {dat, 0, 0, 0, 1, "", eof},
  };
  for (auto& tt : test_cases) {
    CAPTURE(tt.data, tt.off, tt.n, tt.buf_len, tt.at);
    auto r = strings::reader{tt.data};
    auto s = section_reader{r, tt.off, tt.n};
    auto buf = std::vector<uint8_t>(tt.buf_len);
    auto [n, err] = s.read_at(buf, tt.at);
    CHECK(n == static_cast<int>(tt.exp.size()));
    CHECK(bytes::to_string(std::span(buf).subspan(0, n)) == tt.exp);
    CHECK(err == tt.err);
  }
}

TEST_CASE("Section reader seek", "[io]") {
  auto buf = "foo"sv;
  auto br = bytes::reader{bytes::to_bytes(buf)};
  auto sr = section_reader{br, 0, static_cast<int>(buf.size())};

  for (auto whence : std::vector<int>{seek_start, seek_current, seek_end}) {
    for (int64_t offset = -3; offset <= 4; ++offset) {
      CAPTURE(whence, offset);
      auto [br_off, br_err] = br.seek(offset, whence);
      auto [sr_off, sr_err] = sr.seek(offset, whence);
      CHECK(((br_err == nil) == (sr_err == nil)));
      CHECK(br_off == sr_off);
    }
  }

  auto [got, err1] = sr.seek(100, seek_start);
  CHECK(got == 100);
  CHECK(err1 == nil);

  auto b = std::vector<uint8_t>(10);
  auto [n, err2] = sr.read(b);
  CHECK(n == 0);
  CHECK(err2 == eof);
}

TEST_CASE("Section reader size", "[io]") {
  auto test_cases = std::vector<std::tuple<std::string_view, size_t>>{
    {"a long sample data, 1234567890", 30},
    {"", 0},
  };
  for (auto [data, exp] : test_cases) {
    auto r = strings::reader{data};
    auto sr = section_reader{r, 0, static_cast<int>(data.size())};
    CHECK(sr.size() == exp);
  }
}

TEST_CASE("Section reader max", "[io]") {
  auto r = strings::reader{"abcdef"};
  auto sr = section_reader{r, 3, std::numeric_limits<int64_t>::max()};
  auto buf = std::vector<uint8_t>(3);
  auto [n, err] = sr.read(buf);
  CHECK(n == 3);
  CHECK(err == nil);
  std::tie(n, err) = sr.read(buf);
  CHECK(n == 0);
  CHECK(err == eof);
}

struct large_writer {
  std::error_code err = nil;
  std::pair<int, std::error_code> write(std::span<uint8_t const> p) {
    return {static_cast<int>(p.size() + 1), err};
  }
};

TEST_CASE("Copy large writer", "[io]") {
  auto rb = buffer{};
  auto wb = large_writer{};
  rb.write_string("hello, world.");
  auto [_, err] = copy(wb, rb);
  CHECK(err == error::invalid_write);

  auto want = std::make_error_code(std::errc::not_a_socket);
  rb = buffer{};
  wb = large_writer{want};
  rb.write_string("hello, world.");
  std::tie(_, err) = copy(wb, rb);
  CHECK(err == want);
}

}  // namespace bongo::io
