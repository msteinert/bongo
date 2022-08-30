// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include <array>
#include <span>

#include "bongo/bytes.h"
#include "bongo/io.h"
#include "bongo/strings.h"
#include "bongo/testing/iotest.h"

namespace bongo::testing::iotest {

TEST_CASE("One byte Reader", "[testing/iotest]") {
  auto msg = "Hello, World!";
  auto buf = bytes::buffer{};
  buf.write_string(msg);

  auto obr = one_byte_reader{buf};
  auto [n, err] = obr.read({});
  CHECK(n == 0);
  CHECK(err == nil);

  std::array<uint8_t, 3> b;
  auto got = bytes::buffer{};
  for (auto i = 0;; ++i) {
    std::tie(n, err) = obr.read(b);
    if (err != nil) {
      break;
    }
    CHECK(n == 1);
    got.write(std::span{b}.subspan(0, n));
  }
  CHECK(err == io::eof);
  CHECK(got.str() == "Hello, World!");
}

TEST_CASE("Empty one byte Reader", "[testing/iotest]") {
  auto r = bytes::buffer{};

  auto obr = one_byte_reader{r};
  auto [n, err] = obr.read({});
  CHECK(n == 0);
  CHECK(err == nil);

  std::array<uint8_t, 5> b;
  std::tie(n, err) = obr.read(b);
  CHECK(n == 0);
  CHECK(err == io::eof);
}

TEST_CASE("Half Reader", "[testing/iotest]") {
  std::string_view msg = "Hello, World!";
  auto buf = bytes::buffer{};
  buf.write_string(msg);

  auto hr = half_reader{buf};
  auto [n, err] = hr.read({});
  CHECK(n == 0);
  CHECK(err == nil);

  std::array<uint8_t, 2> b;
  auto got = bytes::buffer{};
  for (auto i = 0;; ++i) {
    std::tie(n, err) = hr.read(b);
    if (err != nil) {
      break;
    }
    CHECK(n == 1);
    got.write(std::span{b}.subspan(0, n));
  }
  CHECK(err == io::eof);
  CHECK(got.str() == "Hello, World!");
}

TEST_CASE("Empty half Reader", "[testing/iotest]") {
  auto r = bytes::buffer{};

  auto hr = half_reader{r};
  std::vector<uint8_t> b;
  auto [n, err] = hr.read(b);
  CHECK(n == 0);
  CHECK(err == nil);

  b.resize(5);
  std::tie(n, err) = hr.read(b);
  CHECK(n == 0);
  CHECK(err == io::eof);
}

TEST_CASE("Data error reader, non-empty Reader", "[testing/iotest]") {
  auto buf = bytes::buffer{"Hello, World!"};
  auto der = data_error_reader{buf};

  std::array<uint8_t, 3> b;
  auto got = bytes::buffer{};
  long n;
  std::error_code err;
  for (;;) {
    std::tie(n, err) = der.read(b);
    got.write(std::span{b.begin(), static_cast<size_t>(n)});
    if (err != nil) {
      break;
    }
  }
  CHECK(n != 0);
  CHECK(err == io::eof);
  CHECK(got.str() == "Hello, World!");
}

TEST_CASE("Data error reader, empty Reader", "[testing/iotest]") {
  auto r = bytes::buffer{};

  auto der = data_error_reader{r};
  std::vector<uint8_t> b;
  auto [n, err] = der.read(b);
  CHECK(n == 0);
  CHECK(err == io::eof);

  b.resize(5);
  std::tie(n, err) = der.read(b);
  CHECK(n == 0);
  CHECK(err == io::eof);
}

}  // namespace bongo::testing::iotest
