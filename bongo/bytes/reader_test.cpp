// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include "bongo/bytes.h"
#include "bongo/io.h"
#include "bongo/bytes.h"

using namespace std::string_view_literals;

namespace bongo::bytes {

TEST_CASE("Byte reader", "[bytes]") {
  auto r = reader{to_bytes("0123456789"sv)};
  auto test_cases = std::vector<std::tuple<
    int64_t,          // seek offset
    long,             // seek set
    long,             // read buffer size
    std::string,      // expected read result
    int64_t,          // expected seek return value
    std::error_code,  // expected read error
    std::error_code   // expected seek error
  >>{
    {0, io::seek_start, 20, "0123456789", 0, nil, nil},
    {1, io::seek_start, 1, "1", 1, nil, nil},
    {1, io::seek_current, 2, "34", 3, nil, nil},
    {-1, io::seek_start, 0, "", 0, nil, error::negative_position},
    {1lu << 33, io::seek_start, 0, "", 1lu<<33, io::eof, nil},
    {1, io::seek_current, 0, "", (1lu<<33) + 1, io::eof, nil},
    {0, io::seek_start, 5, "01234", 0, nil, nil},
    {0, io::seek_current, 5, "56789", 5, nil, nil},
    {-1, io::seek_end, 1, "9", 9, nil, nil},
  };
  for (auto& [off, seek, n, want, wantpos, readerr, seekerr] : test_cases) {
    auto [pos, err] = r.seek(off, seek);
    CHECK(err == seekerr);
    CHECK(pos == wantpos);
    if (err) {
      continue;
    }

    auto buf = std::vector<uint8_t>(n);
    std::tie(n, err) = r.read(buf);
    CHECK(err == readerr);
    if (err) {
      continue;
    }

    CHECK(std::string{buf.begin(), std::next(buf.begin(), n)} == want);
  }
}

TEST_CASE("Byte reader read after big seek", "[bytes]") {
  auto r = reader{to_bytes("0123456789"sv)};
  auto [n, err] = r.seek((1lu<<31)+5, io::seek_start);
  REQUIRE(err == nil);

  auto buf = std::vector<uint8_t>(10);
  std::tie(n, err) = r.read(buf);
  CHECK(n == 0);
  CHECK(err == io::eof);
}

TEST_CASE("Byte reader write_to", "[bytes]") {
  auto str = "0123456789"sv;
  for (size_t i = 0; i < str.size(); ++i) {
    auto s = str.substr(i);
    auto r = reader{to_bytes(s)};
    auto b = bytes::buffer{};
    auto [n, err] = r.write_to(b);
    CHECK(static_cast<size_t>(n) == s.size());
    CHECK(err == nil);
    CHECK(b.str() == s);
    CHECK(r.size() == 0);
  }
}

TEST_CASE("Byte reader size/total_size", "[bytes]") {
  auto r = reader{to_bytes("abc"sv)};
  auto w = io::discard{};
  io::copy_n(w, r, 1);
  CHECK(r.size() == 2);
  CHECK(r.total_size() == 3);
}

TEST_CASE("Byte reader reset", "[bytes]") {
  auto r = reader{to_bytes("世界"sv)};
  auto [v, n, err1] = r.read_rune();
  CHECK(err1 == nil);

  auto want = std::string{"abcdef"};
  r.reset(to_bytes(want));
  auto err2 = r.unread_rune();
  CHECK(err2 != nil);

  auto [buf, err3] = io::read_all(r);
  CHECK(err3 == nil);
  CHECK(bytes::to_string(buf) == want);
}

TEST_CASE("Byte reader zero", "[bytes]") {
  CHECK(reader{}.size() == 0);
  CHECK(reader{}.total_size() == 0);

  long n;
  std::error_code err;
  std::tie(n, err)  = reader{}.read(nil);
  CHECK(n == 0);
  CHECK(err == io::eof);

  std::tie(n, err) = reader{}.read_at(nil, 11);
  CHECK(n == 0);
  CHECK(err == io::eof);

  auto w = io::discard{};
  std::tie(n, err) = reader{}.write_to(w);
  CHECK(n == 0);
  CHECK(err == nil);

  uint8_t b;
  std::tie(b, err) = reader{}.read_byte();
  CHECK(b == 0);
  CHECK(err == io::eof);

  rune ch;
  long size;
  std::tie(ch, size, err) = reader{}.read_rune();
  CHECK(ch == 0);
  CHECK(size == 0);
  CHECK(err == io::eof);

  int64_t offset;
  std::tie(offset, err) = reader{}.seek(11, io::seek_start);
  CHECK(offset == 11);
  CHECK(err == nil);

  err = reader{}.unread_byte();
  CHECK(err != nil);

  err = reader{}.unread_rune();
  CHECK(err != nil);
}

}  // namespace bongo::bytes
