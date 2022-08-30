// Copyright The Go Authors.

#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/bytes.h"
#include "bongo/crypto/sha1.h"
#include "bongo/fmt.h"
#include "bongo/io.h"
#include "bongo/strings.h"

namespace sha1 = bongo::crypto::sha1;
using namespace std::string_literals;

namespace bongo::io {

template <typename Function>
void with_foo_bar(Function fn) {
  auto r1 = strings::reader{"foo "};
  auto r2 = strings::reader{""};
  auto r3 = strings::reader{"bar"};
  auto mr = multi_reader{r1, r2, r3};
  fn(mr);
}

template <typename MultiReader>
void expect_read(MultiReader& mr, long size, std::string_view expected, std::error_code eerr) {
  auto buf = std::vector<uint8_t>(size);
  auto [n, gerr] = mr.read(buf);
  CHECK(n == static_cast<long>(expected.size()));
  auto got = bytes::to_string(buf, n);
  CHECK(got == expected);
  CHECK(gerr == eerr);
}

TEST_CASE("Multi reader", "[io]") {
  with_foo_bar([](auto& mr) {
    expect_read(mr, 2, "fo", nil);
    expect_read(mr, 5, "o ", nil);
    expect_read(mr, 5, "bar", nil);
    expect_read(mr, 5, "", eof);
  });
  with_foo_bar([](auto& mr) {
    expect_read(mr, 4, "foo ", nil);
    expect_read(mr, 1, "b", nil);
    expect_read(mr, 3, "ar", nil);
    expect_read(mr, 1, "", eof);
  });
  with_foo_bar([](auto& mr) {
    expect_read(mr, 5, "foo ", nil);
  });
}

TEST_CASE("Multi reader as writer", "[io]") {
  // TODO
}

template <Writer T>
void test_multi_writer(T& sink) {
  auto sha1 = sha1::hash{};
  auto mw = multi_writer{sha1, sink};

  auto source_string = "My input text."s;
  auto source = strings::reader{source_string};
  auto [written, err] = io::copy(mw, source);

  CHECK(written == static_cast<int64_t>(source_string.size()));
  CHECK(err == nil);
  CHECK(fmt::sprintf("%x", sha1.sum()) == "01cb303fa8c30a64123067c5aa6284ba7ec2d31b");
  CHECK(sink.str() == source_string);
}

TEST_CASE("Multi writer", "[io]") {
  // TODO
}

TEST_CASE("Multi writer string", "[io]") {
  auto b = bytes::buffer{};
  test_multi_writer(b);
  // TODO
}

}  // namespace bongo::io
